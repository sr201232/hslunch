#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <time.h>
#include "panel_config.h"
#include "neis_root_ca.h"
#include "meal_types.h"
#if __has_include("wifi_private.h")
#include "wifi_private.h"
#else
static const char DEFAULT_WIFI_SSID[] = "";
static const char DEFAULT_WIFI_PASSWORD[] = "";
#endif

// School verified against NEIS schoolInfo: Seoul, Sangam High School.
constexpr char SCHOOL_CODE[] = "7010806";
constexpr char OFFICE_CODE[] = "B10";
constexpr char AP_NAME[] = "Sangam-Display-2";
constexpr char AP_PASSWORD[] = "Sangam2026!";
constexpr uint32_t REFRESH_MS = 3600000;
constexpr uint32_t RETRY_MS = 60000;
constexpr uint32_t FRAME_MS = 20;
constexpr int BOOT_BUTTON = 0;

MatrixPanel_I2S_DMA *panel = nullptr;
U8G2_FOR_ADAFRUIT_GFX font;
WebServer server(80);
QueueHandle_t mealQueue;
MealData shown = {};
String wifiSsid, wifiPassword, apiKey;
bool panelReady = false, portalOpen = false;
uint32_t lastFrame = 0, lastWifiTry = 0, buttonSince = 0;
uint32_t rebootAt = 0, connectedSince = 0;
uint32_t offlineSince = 0, portalOpenedAt = 0;

class LimitedResponse : public Stream {
 public:
  String text;
  bool overflow = false;
  size_t write(uint8_t value) override { return write(&value, 1); }
  size_t write(const uint8_t *data, size_t size) override {
    if (size > 24000 - text.length()) { overflow = true; return 0; }
    if (!text.concat(reinterpret_cast<const char *>(data), size)) { overflow = true; return 0; }
    return size;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};

bool currentDate(char *out, size_t capacity) {
  time_t now = time(nullptr);
  struct tm local;
  localtime_r(&now, &local);
  if (local.tm_year + 1900 < 2024) { out[0] = 0; return false; }
  strftime(out, capacity, "%Y%m%d", &local);
  return true;
}

String plainText(String text) {
  text.replace("<br/>", " / "); text.replace("<br />", " / ");
  text.replace("<br>", " / "); text.replace("<BR/>", " / ");
  text.replace("&amp;", "&"); text.replace("&nbsp;", " ");
  text.replace("&lt;", "<"); text.replace("&gt;", ">");
  text.trim();
  return text; // Preserve allergy numbers and every parenthesized food name.
}

bool copyText(char *dest, size_t size, const String &text) {
  if (text.length() >= size) return false; // Never truncate UTF-8 or allergy data.
  memcpy(dest, text.c_str(), text.length() + 1);
  return true;
}

String urlEncode(const String &input) {
  const char hex[] = "0123456789ABCDEF";
  String result;
  for (size_t i = 0; i < input.length(); ++i) {
    uint8_t c = input[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') result += char(c);
    else { result += '%'; result += hex[c >> 4]; result += hex[c & 15]; }
  }
  return result;
}

bool decodeMeal(const String &payload, const char *date, MealData &meal) {
  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;
  memset(&meal, 0, sizeof(meal));
  strlcpy(meal.date, date, sizeof(meal.date));
  const char *code = doc["RESULT"]["CODE"] | "";
  if (!strcmp(code, "INFO-200")) {
    meal.noMeal = true;
    strlcpy(meal.menu, "오늘은 등록된 중식 정보가 없습니다", sizeof(meal.menu));
    return true;
  }
  if (*code) return false; // Authentication/quota/server failures are not 'no meal'.
  JsonArray rows = doc["mealServiceDietInfo"][1]["row"].as<JsonArray>();
  for (JsonObject row : rows) {
    if (strcmp(row["MLSV_YMD"] | "", date) ||
        strcmp(row["SD_SCHUL_CODE"] | "", SCHOOL_CODE) ||
        strcmp(row["ATPT_OFCDC_SC_CODE"] | "", OFFICE_CODE) ||
        strcmp(row["MMEAL_SC_CODE"] | "", "2")) continue;
    String menu = plainText(row["DDISH_NM"] | "");
    if (menu.isEmpty()) return false;
    return copyText(meal.menu, sizeof(meal.menu), menu) &&
      copyText(meal.origin, sizeof(meal.origin), plainText(row["ORPLC_INFO"] | "원산지 정보 없음")) &&
      copyText(meal.calories, sizeof(meal.calories), row["CAL_INFO"] | "열량 정보 없음");
  }
  return false;
}

void saveCache(const MealData &meal) {
  Preferences prefs;
  if (!prefs.begin("sangam-cache", false)) return;
  // Store one record atomically. Only write when the actual content changes.
  MealData previous = {};
  bool same = prefs.getBytesLength("meal-v1") == sizeof(meal) &&
    prefs.getBytes("meal-v1", &previous, sizeof(previous)) == sizeof(previous) &&
    memcmp(&previous, &meal, sizeof(meal)) == 0;
  if (!same && prefs.putBytes("meal-v1", &meal, sizeof(meal)) != sizeof(meal))
    Serial.println("[CACHE] write failed");
  prefs.end();
}

bool fetchMeal(const char *date, MealData &meal) {
  WiFiClientSecure tls;
  tls.setCACert(NEIS_ROOT_CA);
  tls.setHandshakeTimeout(12);
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(8000);
  String url = "https://open.neis.go.kr/hub/mealServiceDietInfo?Type=json&pIndex=1&pSize=5";
  url += "&ATPT_OFCDC_SC_CODE=" + String(OFFICE_CODE) + "&SD_SCHUL_CODE=" + SCHOOL_CODE;
  url += "&MMEAL_SC_CODE=2&MLSV_YMD=" + String(date);
  if (!apiKey.isEmpty()) url += "&KEY=" + urlEncode(apiKey);
  if (!http.begin(tls, url)) return false;
  int status = http.GET();
  bool ok = false;
  if (status == HTTP_CODE_OK) {
    // NEIS response is small for a single school/date/meal.
    if (http.getSize() <= 24000) {
      LimitedResponse response;
      int bytes = http.writeToStream(&response);
      if (bytes >= 0 && !response.overflow) ok = decodeMeal(response.text, date, meal);
    }
  }
  Serial.printf("[NEIS] date=%s HTTP=%d parsed=%s\n", date, status, ok ? "OK" : "FAIL");
  http.end();
  return ok;
}

void networkTask(void *) {
  char requestedDate[9] = {};
  uint32_t lastAttempt = 0, interval = RETRY_MS;
  bool attempted = false, wasConnected = false;
  bool timeLogged = false;
  // Network waits must never stop display frames or the setup page.
  MealData *received = new MealData{};
  if (!received) { Serial.println("[NEIS] allocation failed"); vTaskDelete(nullptr); return; }
  for (;;) {
    char today[9];
    bool connected = WiFi.status() == WL_CONNECTED;
    bool clockReady = currentDate(today, sizeof(today));
    if (clockReady && !timeLogged) { Serial.printf("[TIME] KST date=%s\n", today); timeLogged = true; }
    if (connected && clockReady && (!attempted || !wasConnected ||
        strcmp(today, requestedDate) || uint32_t(millis() - lastAttempt) >= interval)) {
      strlcpy(requestedDate, today, sizeof(requestedDate));
      attempted = true;
      bool ok = fetchMeal(today, *received);
      char after[9];
      // Do not publish a response across the midnight boundary.
      if (ok && currentDate(after, sizeof(after)) && !strcmp(today, after)) {
        saveCache(*received);
        xQueueOverwrite(mealQueue, received);
      }
      interval = ok ? REFRESH_MS : RETRY_MS;
      lastAttempt = millis();
    }
    wasConnected = connected;
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

const char SETUP_HTML[] PROGMEM = R"HTML(<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>상암고 급식 전광판</title><style>body{font:17px sans-serif;max-width:460px;margin:40px auto;padding:20px;background:#f3f6f9;color:#17283d}label{display:block;margin-top:22px}input,button{box-sizing:border-box;width:100%;padding:13px;font:inherit;margin-top:7px}button{background:#176d52;color:white;border:0;border-radius:8px}small{display:block;line-height:1.6;color:#4a5868}</style><h1>상암고 급식 전광판</h1><p>처음 한 번만 연결 정보를 저장해 주세요.</p><form method="post" action="/save"><label>2.4GHz Wi-Fi 이름<input name="ssid" maxlength="32" required autocomplete="off"></label><label>Wi-Fi 비밀번호<input name="password" type="password" maxlength="63" autocomplete="new-password"></label><label>나이스 인증키 (선택)<input name="key" maxlength="128" type="password" autocomplete="off"></label><small>인증키 없이도 조회됩니다. 발급받은 키가 있으면 입력하세요. 학교는 서울 상암고등학교, 표시할 급식은 중식으로 설정되어 있습니다.</small><button>저장하고 시작</button></form><p><small>저장 후 설정용 Wi-Fi 연결이 끊어집니다. 인터넷 로그인 화면이 필요한 Wi-Fi 및 기업용 인증은 지원하지 않습니다.</small></p></html>)HTML";

void openPortal() {
  if (portalOpen) return;
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(AP_NAME, AP_PASSWORD)) { Serial.println("[SETUP] AP failed"); return; }
  server.on("/", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", SETUP_HTML);
  });
  server.on("/save", HTTP_POST, [] {
    // Setup is accessible only through the password-protected local AP.
    if (server.client().localIP() != WiFi.softAPIP()) { server.send(403, "text/plain", "AP only"); return; }
    String ssid = server.arg("ssid"), password = server.arg("password"), key = server.arg("key");
    key.trim();
    if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 63 ||
        (!password.isEmpty() && password.length() < 8) || key.length() > 128) {
      server.send(400, "text/plain; charset=utf-8", "Wi-Fi 이름과 비밀번호 길이를 확인하세요."); return;
    }
    Preferences prefs;
    JsonDocument doc;
    doc["ssid"] = ssid; doc["password"] = password; doc["key"] = key;
    String config; serializeJson(doc, config);
    if (!prefs.begin("sangam-cfg", false)) { server.send(500, "text/plain", "Storage unavailable"); return; }
    bool saved = prefs.putString("config", config) == config.length();
    prefs.end();
    if (!saved) { server.send(500, "text/plain", "Save failed"); return; }
    server.send(200, "text/html; charset=utf-8", "<meta name=viewport content='width=device-width,initial-scale=1'><h2>저장했습니다</h2><p>전광판을 다시 시작합니다. Wi-Fi가 연결되지 않으면 1분 후 설정용 Wi-Fi가 다시 나타납니다.</p>");
    rebootAt = millis() + 1500;
  });
  server.onNotFound([] { server.sendHeader("Location", "/"); server.send(302); });
  server.begin();
  portalOpen = true;
  portalOpenedAt = millis();
  Serial.printf("[SETUP] Wi-Fi: %s, URL: http://192.168.4.1\n", AP_NAME);
}

#include "screen_renderer.h"
#include "parser_checks.h"
#include "display_checks.h"

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON, INPUT_PULLUP);
  Serial.println("\n[SANGAM] ESP32 unit 2 / 256x32 / firmware 1.4.0");
  if (!runParserChecks() || !runDisplayChecks()) {
    Serial.println("[FATAL] startup checks failed; network disabled");
    for (;;) delay(1000);
  }
  HUB75_I2S_CFG config(PANEL_WIDTH, PANEL_HEIGHT, 1, PANEL_PINS);
  config.driver = PANEL_DRIVER;
  config.clkphase = PANEL_CLOCK_PHASE;
  config.double_buff = true;
  config.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  config.setPixelColorDepthBits(6);
  panel = new MatrixPanel_I2S_DMA(config);
  panelReady = panel && panel->begin();
  Serial.printf("[DISPLAY] DMA=%s, free heap=%u\n", panelReady ? "OK" : "FAIL", ESP.getFreeHeap());
  if (panelReady) {
    panel->setBrightness8(PANEL_BRIGHTNESS);
    panel->clearScreen();
    setupScreen();
  }
  Preferences prefs;
  wifiSsid = DEFAULT_WIFI_SSID;
  wifiPassword = DEFAULT_WIFI_PASSWORD;
  if (prefs.begin("sangam-cfg", true)) {
    JsonDocument doc;
    if (!deserializeJson(doc, prefs.getString("config", "{}"))) {
      wifiSsid = doc["ssid"] | DEFAULT_WIFI_SSID; wifiPassword = doc["password"] | DEFAULT_WIFI_PASSWORD; apiKey = doc["key"] | "";
    }
    prefs.end();
  }
  if (prefs.begin("sangam-cache", true)) {
    if (prefs.getBytesLength("meal-v1") == sizeof(shown)) prefs.getBytes("meal-v1", &shown, sizeof(shown));
    prefs.end();
    shown.date[8] = 0; shown.menu[sizeof(shown.menu)-1] = 0;
    shown.origin[sizeof(shown.origin)-1] = 0; shown.calories[sizeof(shown.calories)-1] = 0;
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("sangam-display-2");
  WiFi.setAutoReconnect(true);
  if (wifiSsid.isEmpty()) openPortal();
  else { WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str()); lastWifiTry = millis(); }
  configTime(9 * 3600, 0, "time.google.com", "pool.ntp.org", "time.cloudflare.com");
  mealQueue = xQueueCreate(1, sizeof(MealData));
  if (!mealQueue || xTaskCreatePinnedToCore(networkTask, "neis", 16384, nullptr, 1, nullptr, 0) != pdPASS)
    Serial.println("[FATAL] network task allocation failed");
  Serial.printf("[READY] school=%s meal=2 free heap=%u\n", SCHOOL_CODE, ESP.getFreeHeap());
}

void loop() {
  uint32_t now = millis();
  if (rebootAt && int32_t(now - rebootAt) >= 0) ESP.restart();
  if (portalOpen) server.handleClient();
  if (digitalRead(BOOT_BUTTON) == LOW) {
    if (!buttonSince) buttonSince = now;
    if (uint32_t(now - buttonSince) >= 5000) openPortal();
  } else buttonSince = 0;
  if (!wifiSsid.isEmpty() && WiFi.status() != WL_CONNECTED) {
    connectedSince = 0;
    if (!offlineSince) offlineSince = now;
    if (uint32_t(now - offlineSince) >= 60000) openPortal();
    if (uint32_t(now - lastWifiTry) >= 30000) {
      WiFi.reconnect(); lastWifiTry = now;
      Serial.printf("[WIFI] reconnecting status=%d\n", WiFi.status());
    }
  } else if (WiFi.status() == WL_CONNECTED) {
    offlineSince = 0;
    if (!connectedSince) { connectedSince = now; Serial.printf("[WIFI] connected %s\n", WiFi.localIP().toString().c_str()); }
    if (portalOpen && buttonSince == 0 && uint32_t(now - connectedSince) >= 120000 && uint32_t(now - portalOpenedAt) >= 120000) {
      server.stop(); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA); portalOpen = false;
    }
  }
  if (mealQueue && xQueueReceive(mealQueue, &shown, 0) == pdTRUE) {
    // Commit new data at the next page boundary; never interrupt a sentence.
    Serial.printf("[MEAL] received date=%s noMeal=%d\n", shown.date, shown.noMeal);
  }
  if (panelReady && uint32_t(now - lastFrame) >= FRAME_MS) { lastFrame = now; renderFrame(); }
  delay(2);
}

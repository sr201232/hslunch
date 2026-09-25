#pragma once
#include "display_sequence.h"

struct ScreenPage {
  String text;
  int width = 0;
  uint8_t number = 0;
  char mealDate[9] = {};
};
#include "sangam18.h"
#include "origin_format.h"
ScreenPage activePage, incomingPage;
bool sequenceStarted = false, transitioning = false;
uint32_t pageBegan = 0, transitionBegan = 0;

uint32_t readCodepoint(const char *&p) {
  uint8_t c = uint8_t(*p++);
  if (c < 128) return c;
  unsigned n = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
  uint32_t value = c & (n == 1 ? 31 : n == 2 ? 15 : 7);
  while (n-- && *p) value = (value << 6) | (uint8_t(*p++) & 63);
  return value;
}
int textWidth(const String &text) {
  int width = 0; const char *p = text.c_str();
  while (*p) width += glyphAdvance(readCodepoint(p));
  return width;
}
#include "origin_pages.h"

uint16_t pageColor(uint8_t page) {
  if (page == 8) return panel->color565(255,255,255);
  return page % 2 == 0 ? panel->color565(0,255,0) : panel->color565(255,255,0);
}
void drawGlyph(uint32_t code, int x, int y, uint16_t color) {
  int index = fontIndex(code);
  for (int row=0; row<24; ++row) {
    if (y+row < 0 || y+row >= PANEL_HEIGHT) continue;
    for (int col=0; col<20; ++col) {
      if (x+col < 0 || x+col >= PANEL_WIDTH) continue;
      int bit = row*20+col;
      if (pgm_read_byte(&SANGAM18[index][1+bit/8]) & (0x80 >> (bit%8)))
        panel->drawPixel(x+col,y+row,color);
    }
  }
}

String clockText(bool colon) {
  time_t now = time(nullptr);
  struct tm local;
  localtime_r(&now, &local);
  if (local.tm_year + 1900 < 2024) return "----.--.-- --:--";
  char stamp[24];
  strftime(stamp, sizeof(stamp), "%Y.%m.%d %H:%M", &local);
  if (!colon) stamp[13] = ' '; // Keep colon cell width fixed.
  return String(stamp);
}

ScreenPage makePage(uint8_t number) {
  ScreenPage page;
  page.number = number;
  if (number == 0) page.text = clockText(true);
  else if (number == 1) page.text = PROJECT_CREDIT;
  else if (number == 2) page.text = "급식 정보";
  else if (number == 4) {
    prepareOriginPages();
    page.text = "원산지 정보";
    if (originParts.size()>1) page.text += " " + String(originCursor+1) + "/" + String(originParts.size());
  }
  else if (number == 6) page.text = "칼로리 정보";
  else if (number == 8) page.text = AD_TEXT;
  else {
    char today[9];
    bool timed = currentDate(today, sizeof(today));
    bool usable = shown.date[0] && (!timed || !strcmp(today, shown.date));
    if (!usable) page.text = wifiSsid.isEmpty() ? "Wi-Fi 설정 필요" : (timed ? "오늘 급식 조회 중" : "시간 확인 중");
    else {
      strlcpy(page.mealDate, shown.date, sizeof(page.mealDate));
      if (shown.noMeal) page.text = "등록된 중식 정보 없음";
      else if (number == 3) page.text = String(shown.menu);
      else if (number == 7) page.text = String(shown.calories);
      else { prepareOriginPages(); page.text = originParts[originCursor]; }
      if (!timed) page.text = "시간 미확인 / 저장 " + String(shown.date) + " / " + page.text;
    }
  }
  page.width = number == 0 ? DisplaySequence::CLOCK_WIDTH : textWidth(page.text);
  return page;
}

void drawPageText(const ScreenPage &page, int offsetX, int offsetY, uint32_t elapsed) {
  String clock;
  const String *text = &page.text;
  if (page.number == 0) { clock = clockText(true); text = &clock; }
  const char *p = text->c_str();
  int x = offsetX, position = 0;
  while (*p && x < PANEL_WIDTH) {
    uint32_t code = readCodepoint(p);
    int advance = page.number == 0 ? DisplaySequence::clockAdvance(code) : glyphAdvance(code);
    if (x+20 > 0 && !(page.number == 0 && position == 13 && !DisplaySequence::colonVisible(elapsed)))
      drawGlyph(code,x,(PANEL_HEIGHT-24)/2+offsetY,pageColor(page.number));
    x += advance; ++position;
  }
}

void drawPage(const ScreenPage &page, int offsetX, int offsetY, uint32_t elapsed) {
  drawPageText(page,offsetX,offsetY,elapsed);
}

void startPage(ScreenPage page, uint32_t now) {
  if (page.number == 6 && activePage.number == 5 && !originParts.empty())
    originCursor = (originCursor+1) % originParts.size();
  activePage = page;
  pageBegan = now;
  transitioning = false;
  Serial.printf("[PAGE] %u width=%d hold+scroll=%lu ms\n", activePage.number + 1, activePage.width,
    (unsigned long)DisplaySequence::duration(activePage.number, activePage.width));
}

void setupScreen() {}

void renderFrame() {
  using namespace DisplaySequence;
  uint32_t now = millis();
  if (!sequenceStarted) { startPage(makePage(0), now); sequenceStarted = true; }
  char today[9];
  if (currentDate(today, sizeof(today))) {
    // Invalidate yesterday's menu even in the middle of a long scroll.
    if (activePage.mealDate[0] && strcmp(today, activePage.mealDate)) startPage(makePage(activePage.number), now);
    if (transitioning && incomingPage.mealDate[0] && strcmp(today, incomingPage.mealDate)) incomingPage = makePage(incomingPage.number);
  }
  uint32_t elapsed = uint32_t(now - pageBegan);
  if (!transitioning && elapsed >= duration(activePage.number, activePage.width)) {
    incomingPage = makePage(next(activePage.number));
    if (slidesUp(activePage.number)) { transitioning = true; transitionBegan = now; }
    else startPage(incomingPage, now);
    elapsed = uint32_t(now - pageBegan);
  }
  panel->fillScreen(0);
  if (transitioning) {
    uint32_t sliding = uint32_t(now - transitionBegan);
    if (sliding >= SLIDE_MS) {
      startPage(incomingPage, now);
      drawPage(activePage, x(activePage.number,activePage.width,0),0,0);
    } else {
      int y = slideY(sliding);
      uint32_t end = duration(activePage.number, activePage.width);
      drawPage(activePage,x(activePage.number,activePage.width,end),y,end);
      drawPage(incomingPage,x(incomingPage.number,incomingPage.width,0),y+HEIGHT,0);
    }
  } else drawPage(activePage,x(activePage.number,activePage.width,elapsed),0,elapsed);
  panel->flipDMABuffer();
}

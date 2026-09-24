#상암고 급식정보 전광판 프로젝트

재학중인 상암고등학교의 중식 메뉴, 알레르기 번호, 원산지, 칼로리 등을 나이스 API에서 받아 LED 전광판에 표시, 급식실 앞에 게시하는 개인 프로젝트.

**제작: 김아준**
**구상 기간: 2026.08.27 - 2026.09.17**
**제작 기간: 2026.09.18 - 2026.09.28**
**게시: 2026.09.30 - **
제작기: https://sr20123.tistory.com/6

- 서울특별시교육청 상암고등학교: 교육청 `B10`, 학교 `7010806`, **중식**.
- 공개 소스에는 실제 Wi-Fi 연결 정보가 없습니다. 첫 실행 때 설정용 Wi-Fi로 연결하거나 `wifi_private.example.h`를 `wifi_private.h`로 복사해 본인의 2.4GHz Wi-Fi를 입력합니다. 실제 비밀번호 파일은 Git에서 제외됩니다.
- 한국 시각을 맞춘 뒤 오늘 급식을 조회합니다. 아래 9페이지를 모두 화면 중앙 한 줄로 표시합니다.
- 정상 조회는 1시간 간격, 실패는 1분 간격으로 재시도합니다. 날짜 변경과 Wi-Fi 재연결 시 즉시 재조회합니다.
- 마지막 성공 자료는 전원을 꺼도 보관합니다. 시간이 확인되면 당일 자료만 표시합니다. 시간이 미확인일 때 저장 자료를 표시하면 날짜와 `저장`, `시간 미확인`을 명시합니다.
- 급식이 등록되지 않은 날과 통신 오류를 구분합니다. 통신 오류를 '급식 없음'으로 표시하지 않습니다.
- 급식이 갱신되어도 읽고 있던 문장을 중간에 끊지 않고 다음 페이지 진입 시 최신 자료를 사용합니다. 자정이 지나 전날 자료가 된 경우에는 즉시 조회 안내로 바꿉니다.

## 표출 순서

| 페이지 | 내용 |
| 1 | 현재 날짜, 시간 |
| 2 | `급식정보 전광판 프로젝트 by 30805 김아준, github.com/sr201232/hslunch` 고정 텍스트 |
| 3 | `급식 정보` 고정 텍스트 |
| 4 | 메뉴 + 음식별 알레르기 번호 |
| 5 | `원산지 정보` 고정 텍스트 |
| 6 | 식재료 별 원산지 정보 일부 |
| 7 | `칼로리 정보` 고정 텍스트 |
| 8 | 나이스 칼로리 값 |
| 9 | 계도 문구 |

# READ ME 제작중!!!!

1. 휴대폰을 `Sangam-Display-2`에 연결합니다. 설정용 비밀번호: `Sangam2026!`
2. 브라우저에서 `http://192.168.4.1`을 엽니다. 인터넷이 없는 Wi-Fi에 계속 연결하도록 선택합니다.
3. 새 Wi-Fi 이름/비밀번호를 저장합니다. 나이스 인증키는 선택 사항입니다.

새 설정은 보드 내부에 저장되어 소스의 기본 Wi-Fi보다 우선합니다. 일반적인 2.4GHz 개인용 Wi-Fi를 지원합니다. 웹 로그인/추가 동의가 필요한 망이나 기업용 인증은 별도 대응이 필요합니다.

## 하드웨어와 배선

ESP32-DevKitC / WROOM-32E, 16MB Flash, PSRAM 없음. USB에서 ESP32-D0WD-V3 / 16MB를 확인했습니다. 포트는 운영체제와 연결 상태에 따라 다르므로 Arduino IDE에서 확인합니다.

일반적인 **256×32, 1/16 스캔, SHIFTREG HUB75**를 기준으로 작성했습니다. 같은 높이의 64×32 네 장을 가로 직렬 연결한 구성도 동일한 256픽셀 데이터로 구동합니다. 패널 설정은 `panel_config.h`에 모았습니다. 실제 패널의 특수 스캔 방식·드라이버가 다르면 이 설정 또는 픽셀 매핑을 조정해야 합니다.

| 신호 | ESP32 GPIO |
|---|---:|
| R1 / G1 / B1 | 25 / 26 / 27 |
| R2 / G2 / B2 | 14 / 12 / 13 |
| A / B / C / D | 23 / 19 / 5 / 17 |
| E | 미사용 |
| LAT / OE / CLK | 4 / 15 / 16 |
| GND | 공통 GND |

패널의 **입력(IN)** 단자에 연결합니다. 패널 5V 전원은 SMPS에서 직접 공급하고 ESP32·패널·SMPS의 GND를 공통으로 연결합니다. 밝기 초기값은 48/255입니다. 화면의 색이나 위치가 잘못되면 `PANEL_CLOCK_PHASE`, 드라이버/스캔 규격을 확인합니다. 실제 패널 표시 품질은 SMPS 연결 후 확인해야 합니다.

## 빌드 설정

- Arduino ESP32 core 3.3.10 / ESP32 Dev Module
- Flash Size 16MB / Flash Mode DIO / PSRAM Disabled
- Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
- ESP32 HUB75 LED MATRIX PANEL DMA Display 3.0.14
- U8g2_for_Adafruit_GFX 1.8.0
- ArduinoJson 7.4.3 / Adafruit GFX 및 BusIO

나이스 서버는 HTTPS 루트 인증서로 검증합니다. 서버가 다른 인증기관으로 전환되면 `neis_root_ca.h` 갱신이 필요할 수 있습니다. Wi-Fi 비밀번호가 담긴 `wifi_private.h`와 업로드용 바이너리는 공개하지 마세요. 이 헤더를 제외하고 배포하면 첫 실행 시 설정용 Wi-Fi를 이용합니다.

## 확인한 자료

- [나이스 학교 조회](https://open.neis.go.kr/hub/schoolInfo?Type=json&ATPT_OFCDC_SC_CODE=B10&SD_SCHUL_CODE=7010806)
- [나이스 중식 조회 예시](https://open.neis.go.kr/hub/mealServiceDietInfo?Type=json&ATPT_OFCDC_SC_CODE=B10&SD_SCHUL_CODE=7010806&MLSV_YMD=20260922&MMEAL_SC_CODE=2)
- [HUB75 구동 라이브러리](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA)
- [학교급식 알레르기 번호 안내](https://dohyeon-e.goeyi.kr/dohyeon-e/ad/fm/foodmenu/selectFoodMenuView.do?mi=23356)

컴파일·업로드·실제 부팅 확인 결과는 `verification.md`에 기록합니다.

현재 해상도는 사용자가 추정한 256×32 기준입니다. P4의 32픽셀 높이는 12.8cm, 256픽셀 폭은 102.4cm입니다. 실제 가로 길이는 아직 확인하지 않았습니다.

## 글꼴 재생성

일반 빌드는 포함된 `sangam18.h`를 그대로 사용하므로 글꼴 도구를 실행할 필요가 없습니다. 글꼴을 바꾸려면 Python과 Pillow를 설치하고 ZIP의 `font-tools/generate_font.py`를 실행합니다. 함께 제공한 원본 `NotoSansKR.ttf`와 `OFL.txt`를 사용합니다. 글꼴 출처: https://github.com/google/fonts/tree/main/ofl/notosanskr . 미리보기에도 같은 비트맵을 사용하되 실제 LED 밝기·촬영 차이는 재현하지 않습니다.

## 속도와 원산지 분할

모든 가로 스크롤은 초당 42픽셀로 통일했습니다. 2·4·6·8페이지는 시작·끝에 각각 1초 정지하고, 마지막 문구 페이지는 정지 없이 48픽셀 간격으로 이어 붙여 총 10초 동안 반복합니다. 위로 넘어가는 페이지 전환은 기존 0.4초를 유지합니다.

원산지는 한 회차에 한 구간만 표시합니다. 제목에 `원산지 정보 1/4`처럼 진행 위치가 나오고 다음 회차에 다음 구간으로 넘어갑니다. 구간 길이는 최대 15초이며 급식 메뉴 표시 시간보다 길지 않습니다. 같은 국가의 긴 식재료 목록은 국가명을 다시 붙여 나눕니다. 지나치게 긴 단일 항목은 UTF-8 글자를 자르지 않고 연속 구간으로 나눕니다. 내용은 삭제하지 않으며, 날짜나 데이터가 바뀌면 첫 구간부터 다시 시작합니다. 재부팅하면 역시 첫 구간부터 표시합니다.

시계는 `%Y.%m.%d %H:%M` 형식으로 날짜와 시간 사이가 정확히 한 칸입니다. 숫자 폭은 고정하고 기호·공백 폭을 따로 지정하며 콜론 칸만 점멸합니다. 예시 날짜는 형식을 설명하기 위한 것이며 실제 시각은 NTP로 맞춘 한국 시각입니다.

## 오픈소스

프로젝트 시작: 2026.8.27(목). 기존 안내: https://ajmk.kr/4 .
공개 저장소: https://github.com/sr201232/hslunch .
프로젝트 코드와 설명은 MIT 라이선스입니다. Noto Sans KR 원본과 파생 비트맵 Sangam18은 SIL OFL 1.1이며 `font-tools/OFL.txt`를 따릅니다. 외부 Arduino 라이브러리는 각각의 라이선스를 따릅니다.

## Arduino IDE로 시작하기

1. 보드 관리자 URL에 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`을 추가하고 ESP32 core 3.3.10을 설치합니다.
2. 위 빌드 설정에 적힌 라이브러리를 라이브러리 관리자에서 설치합니다.
3. `sangam_display/sangam_display.ino`를 열고 ESP32 Dev Module, 16MB Flash, DIO, PSRAM Disabled, 3MB APP/9.9MB FATFS 파티션을 선택합니다.
4. `panel_config.h`의 해상도·GPIO를 실물과 맞춘 뒤 업로드합니다. LED 전원은 패널 요구 사양에 맞는 별도 전원을 사용하며 ESP32와 GND를 공유합니다.
5. 첫 부팅의 Wi-Fi 설정을 완료합니다. 공개 코드에 실제 비밀번호·인증키를 커밋하지 마세요.

Arduino CLI를 사용하는 경우:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=disabled,FlashMode=dio sangam_display
arduino-cli upload --port YOUR_PORT --fqbn esp32:esp32:esp32:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=disabled,FlashMode=dio sangam_display
```

## 구성

- `sangam_display/`: 펌웨어, 패널 설정, 한글 도트 데이터, 원산지 분할, 부팅 자체 검사.
- `font-tools/`: 원본 OFL 글꼴과 비트맵 재생성 도구. 일반 빌드에는 Python이 필요하지 않습니다.
- `CHANGELOG.md`: 버전별 변경 내용.
- `verification.md`: 확인한 내용과 아직 실기 확인이 필요한 범위.
- `LICENSE`: 프로젝트 코드의 MIT 라이선스. 글꼴에는 별도 OFL 적용.

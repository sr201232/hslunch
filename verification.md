# 검증 기록 — v1.6.0 / 2026-09-25

ESP32 2호기(COM7)에 업로드 완료했습니다.

- 컴파일 성공: 프로그램 1,847,272 / 3,145,728바이트(58%), 전역 메모리 57,240 / 327,680바이트(17%).
- ESP32-D0WD-V3 rev 3.1 식별 후 기록, 플래시 해시 검증 및 재부팅 성공.
- 실제 보드 부팅 검사: 데이터 8/8, 표출 17/17 통과. HUB75 DMA 초기화 성공.
- 직렬 로그에서 1~7페이지 및 1페이지 복귀 확인.
- 미리보기 실행 검사: 정확한 소개 문구, 원산지 제거, 7페이지 순환, 45px/s, 색상, 시계 간격, 전환 및 스크롤 종료 통과.
- 학교 Wi-Fi 범위 밖에서 연결 재시도를 확인했습니다. 실시간 당일 급식 수신과 시간 동기화는 이번 환경에서 확인하지 못했습니다.

```text
[SANGAM] ESP32 unit 2 / 256x32 / firmware 1.6.0
[CHECK] parser 8/8 passed
[CHECK] display 17/17 passed
[DISPLAY] DMA=OK, free heap=155164
[READY] school=7010806 meal=2 free heap=79652
[PAGE] 1 width=154 hold+scroll=2000 ms
[PAGE] 2 width=564 hold+scroll=8845 ms
[PAGE] 3 width=77 hold+scroll=1000 ms
[PAGE] 4 width=1574 hold+scroll=31289 ms
[PAGE] 5 width=95 hold+scroll=1000 ms
[PAGE] 6 width=356 hold+scroll=4223 ms
[PAGE] 7 width=268 hold+scroll=2267 ms
[PAGE] 1 width=154 hold+scroll=2000 ms
```

실물 발광·색상·배선·스캔 배열은 관찰하지 않았습니다. 미리보기와 펌웨어는 사용자 추정 256×32(8:1) 기준입니다. P4 높이 32픽셀은 12.8cm이며 실제 가로 길이는 미확인입니다.

나이스 정상 응답 INFO-200의 급식 미등록 처리 검사는 통과했습니다. 메뉴·칼로리 페이지에 등록된 중식 정보 없음으로 안내하며, 통신 실패를 급식 없음으로 표시하지 않습니다.

공개 저장소 README.md는 보존합니다. 변경 규칙은 docs/firmware-v1.6.md에 기록합니다.
로컬 앱 바이너리: 1,847,424바이트.
SHA-256: 4EB02E8DF2FE18029FBA191E77318AD53F92C95B370619F4F405AA38AEE2564F
개인 Wi-Fi 설정과 이를 포함한 바이너리는 공개하지 않습니다.

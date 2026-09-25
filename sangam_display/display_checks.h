#pragma once
bool runDisplayChecks() {
  using namespace DisplaySequence;
  unsigned passed = 0;
  if (duration(0,119) == 2000 && duration(8,128) == 2000) ++passed;
  if (duration(2,72) == 1000 && duration(4,120) == 1000 && duration(6,72) == 1000) ++passed;
  if (x(3,456,0) == 0 && x(3,456,999) == 0 && x(3,456,1000) == 0) ++passed;
  if (x(3,456,1040) == -1 && x(3,456,2000) == -45) ++passed;
  if (duration(3,456) == 6445 && x(3,456,5445) + 456 == 256 && x(3,456,9999) == -200) ++passed;
  if (duration(1,80) == 2000 && x(1,80,0) == 88 && x(1,80,1999) == 88) ++passed;
  bool order = true; uint8_t page = 0;
  for (uint8_t i = 0; i < 9; ++i) { order &= page == i; page = next(page); }
  if (order && page == 0) ++passed;
  bool transitions = true;
  for (uint8_t i = 0; i < 9; ++i) transitions &= slidesUp(i) == (i < 7);
  if (transitions && slideY(0) == 0 && slideY(200) == -16 && slideY(400) == -32) ++passed;
  unsigned rises = 0, falls = 0; bool previous = false;
  for (uint32_t ms = 0; ms < 2000; ++ms) {
    bool visible = colonVisible(ms);
    if (visible && !previous) ++rises;
    if (!visible && previous) ++falls;
    previous = visible;
  }
  if (rises == 3 && falls == 3) ++passed;
  uint32_t start = 0xffffff00U, end = start + 2000U;
  if (uint32_t(end - start) == 2000 && !colonVisible(uint32_t(end - start))) ++passed;
  if (x(7,32760,0) == 0 && x(7,32760,duration(7,32760)) + 32760 == 256) ++passed;
  if (scrolls(1) && scrolls(3) && scrolls(5) && scrolls(7) && scrolls(8)) ++passed;
  if (groupOrigins("쌀: 국내산 / 배추: 국내산 / 김치: 중국산") == "국내산: 쌀, 배추 / 중국산: 김치") ++passed;
  if (groupOrigins("쇠고기: 국내산(육우) / 가공품: 국내산, 호주산 / 명태: 러시아산(국내가공)") == "국내산(육우): 쇠고기 / 국내산, 호주산: 가공품 / 러시아산(국내가공): 명태") ++passed;
  if (groupOrigins("정보 없음 / 쌀: / : 국내산") == "정보 없음 / 쌀: / : 국내산") ++passed;
  if (groupOrigins(" 쌀 : 국내산 / 배추 : 국내산 ") == "국내산: 쌀, 배추") ++passed;
  if (fontIndex(0xac00)==189 && fontIndex(0xd7a3)==11360 && glyphAdvance('A')>0) ++passed;
  if (speed(3)==45 && speed(5)==45 && speed(1)==45 && speed(8)==45) ++passed;
  if (duration(8,456)==6445 && x(8,456,5445)==-200 && x(8,456,9999)==-200) ++passed;
  String raw="쌀: 국내산 / 배추: 국내산 / 돼지고기: 국내산 / 닭고기: 국내산 / 오리고기: 국내산 / 김치: 중국산";
  auto parts=splitOriginPages(raw);
  if (parts.size()==2 && parts[0]=="쌀: 국내산 / 배추: 국내산 / 돼지고기: 국내산" && parts[0]+" / "+parts[1]==raw) ++passed;
  if (splitOriginPages("쇠고기: 국내산(육우) / 명태: 러시아산(국내가공)").size()==1) ++passed;
  struct tm sample = {}; sample.tm_year=126; sample.tm_mon=7; sample.tm_mday=8; sample.tm_hour=11; sample.tm_min=57;
  char stamp[24]; strftime(stamp,sizeof(stamp),"%Y.%m.%d %H:%M",&sample);
  if (String(stamp)=="2026.08.08 11:57" && stamp[13]==':') ++passed;
  int clockWidth=0; for (unsigned i=0; stamp[i]; ++i) clockWidth += clockAdvance(stamp[i]);
  if (clockWidth==CLOCK_WIDTH && clockAdvance(':')==5 && clockAdvance(' ')==7) ++passed;
  Serial.printf("[CHECK] display %u/23 passed\n", passed);
  return passed == 23;
}

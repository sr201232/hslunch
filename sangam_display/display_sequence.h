#pragma once
#include <stdint.h>

// All durations exclude the upward transition itself.
namespace DisplaySequence {
constexpr int WIDTH = 256;
constexpr int HEIGHT = 32;
constexpr uint32_t HOLD_MS = 1000;
constexpr uint32_t SLIDE_MS = 400;
constexpr int CLOCK_WIDTH = 154;
inline int clockAdvance(uint32_t c) { return c=='.' || c==':' ? 5 : (c==' ' ? 7 : 11); }
inline uint32_t speed(uint8_t page) {
  return 45;
}
constexpr uint8_t PAGE_COUNT = 7;

inline bool scrolls(uint8_t page) {
  return page == 1 || page == 3 || page == 5 || page == 6;
}
inline uint8_t next(uint8_t page) { return (page + 1) % PAGE_COUNT; }
inline bool slidesUp(uint8_t from) { return from < 5; }
inline int travel(int width) { return width > WIDTH ? width - WIDTH : 0; }
inline uint32_t duration(uint8_t page, int width) {
  if (scrolls(page))
    return HOLD_MS * 2 + (uint32_t(travel(width)) * 1000 + speed(page) - 1) / speed(page);
  if (page == 0) return 2000;
  return 1000;
}
inline int x(uint8_t page, int width, uint32_t elapsed) {
  if (width <= WIDTH) return (WIDTH - width) / 2;
  if (!scrolls(page)) return 0;
  if (elapsed <= HOLD_MS) return 0;
  uint32_t distance = uint64_t(elapsed - HOLD_MS) * speed(page) / 1000;
  return -int(distance > uint32_t(travel(width)) ? travel(width) : distance);
}
inline bool colonVisible(uint32_t elapsed) {
  // Exactly three complete on/off cycles during the two-second clock page.
  if (elapsed >= 2000) elapsed = 1999;
  return ((elapsed * 6 / 2000) % 2) == 0;
}
inline int slideY(uint32_t elapsed) {
  return elapsed >= SLIDE_MS ? -HEIGHT : -int(elapsed * HEIGHT / SLIDE_MS);
}
}

constexpr char PROJECT_CREDIT[] = "급식정보 전광판 프로젝트 v1 by 30805김아준, GitHub @sr201232";
constexpr char AD_TEXT[] = "지성: 지극과 정성 / 상암고등학교";

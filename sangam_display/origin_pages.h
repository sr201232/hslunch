#pragma once
#include <vector>

// Helpers are included after textWidth/readCodepoint. Never drop a source item.
inline int originWidthLimit(int menuWidth) {
  int budget = menuWidth < DisplaySequence::WIDTH ? DisplaySequence::WIDTH : menuWidth;
  const int cap = DisplaySequence::WIDTH + 13 * DisplaySequence::speed(5);
  return budget < cap ? budget : cap;
}
inline void appendOriginPart(std::vector<String> &parts, const String &text, int limit) {
  String chunk;
  const char *p = text.c_str();
  while (*p) {
    const char *begin = p;
    readCodepoint(p);
    String glyph;
    while (begin < p) glyph += *begin++;
    if (chunk.length() && textWidth(chunk + glyph) > limit) {
      parts.push_back(chunk); chunk = "";
    }
    chunk += glyph;
  }
  if (chunk.length()) parts.push_back(chunk);
}
inline std::vector<String> splitOriginPages(const String &grouped, int limit) {
  std::vector<String> parts;
  unsigned start = 0;
  while (start < grouped.length()) {
    int end = grouped.indexOf(" / ", start);
    if (end < 0) end = grouped.length();
    String group = grouped.substring(start,end);
    int colon = group.indexOf(':');
    if (colon < 1) appendOriginPart(parts,group,limit);
    else {
      String prefix = group.substring(0,colon) + ": ", chunk, item;
      int depth = 0;
      for (unsigned i=colon+1; i<=group.length(); ++i) {
        char c = i<group.length() ? group[i] : ',';
        if (c=='(') ++depth;
        if (c==')' && depth) --depth;
        if ((c==',' && depth==0) || i==group.length()) {
          item.trim();
          if (item.length()) {
            String candidate = chunk.length() ? chunk + ", " + item : prefix + item;
            if (chunk.length() && textWidth(candidate)>limit) {
              appendOriginPart(parts,chunk,limit); candidate = prefix + item;
            }
            chunk = candidate;
          }
          item = "";
        } else item += c;
      }
      if (chunk.length()) appendOriginPart(parts,chunk,limit);
      else appendOriginPart(parts,group,limit);
    }
    start = end+3;
  }
  if (parts.empty()) parts.push_back(grouped);
  return parts;
}

std::vector<String> originParts;
String originSource, originDate;
int originLimit = 0;
unsigned originCursor = 0;
void prepareOriginPages() {
  int limit = originWidthLimit(textWidth(String(shown.menu)));
  if (originParts.empty() || originSource != shown.origin || originDate != shown.date || originLimit != limit) {
    originSource = shown.origin; originDate = shown.date; originLimit = limit;
    originParts = splitOriginPages(groupOrigins(originSource),limit);
    originCursor = 0;
  }
}

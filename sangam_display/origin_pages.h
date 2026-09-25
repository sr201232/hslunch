#pragma once
#include <vector>
// Keep the original NEIS ingredient/origin pairs intact.
inline std::vector<String> splitOriginPages(const String &raw) {
  std::vector<String> entries, parts;
  unsigned start=0;
  while (start<raw.length()) {
    int end=raw.indexOf(" / ",start);
    if (end<0) end=raw.length();
    String item=raw.substring(start,end); item.trim();
    if (item.length()) entries.push_back(item);
    start=end+3;
  }
  if (entries.empty()) { parts.push_back(raw); return parts; }
  unsigned groups=(entries.size()+4)/5;
  unsigned base=entries.size()/groups, extra=entries.size()%groups, index=0;
  for (unsigned group=0; group<groups; ++group) {
    String text;
    unsigned count=base+(group<extra ? 1 : 0);
    for (unsigned i=0;i<count;++i) {
      if (text.length()) text += " / ";
      text += entries[index++];
    }
    parts.push_back(text);
  }
  return parts;
}
std::vector<String> originParts;
String originSource, originDate;
unsigned originCursor=0;
void prepareOriginPages() {
  if (originParts.empty() || originSource!=shown.origin || originDate!=shown.date) {
    originSource=shown.origin; originDate=shown.date;
    originParts=splitOriginPages(originSource); originCursor=0;
  }
}

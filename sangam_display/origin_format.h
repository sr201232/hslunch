#pragma once
// Group only exact origin labels. Mixed origins and parenthetical qualifiers
// remain intact so grouping cannot falsely assign a food to one country.
String groupOrigins(const String &raw) {
  String keys[64], foods[64], unparsed;
  unsigned count = 0, start = 0;
  while (start < raw.length()) {
    int end = raw.indexOf(" / ", start);
    if (end < 0) end = raw.length();
    String part = raw.substring(start, end); part.trim();
    int colon = part.indexOf(':');
    String food = colon > 0 ? part.substring(0, colon) : String(); food.trim();
    String origin = colon > 0 ? part.substring(colon + 1) : String(); origin.trim();
    if (!food.length() || !origin.length()) {
      if (part.length()) { if (unparsed.length()) unparsed += " / "; unparsed += part; }
    } else {
      unsigned group = 0;
      while (group < count && keys[group] != origin) ++group;
      if (group == count) {
        if (count == 64) return raw; // Preserve all information on overflow.
        keys[count++] = origin;
      }
      if (foods[group].length()) foods[group] += ", ";
      foods[group] += food;
    }
    start = end + 3;
  }
  String output;
  for (unsigned i=0; i<count; ++i) {
    if (output.length()) output += " / ";
    output += keys[i] + ": " + foods[i];
  }
  if (unparsed.length()) { if (output.length()) output += " / "; output += unparsed; }
  return output;
}

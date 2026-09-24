#pragma once
// Small deterministic boot checks for the data boundary. No network or NVS writes.
bool runParserChecks() {
  MealData *meal = new MealData{};
  if (!meal) return false;
  unsigned passed = 0;
  const char *date = "20260922";
  String good = R"JSON({"mealServiceDietInfo":[{}, {"row":[{"ATPT_OFCDC_SC_CODE":"B10","SD_SCHUL_CODE":"7010806","MLSV_YMD":"20260922","MMEAL_SC_CODE":"2","DDISH_NM":"밥<br/>국 (1.5.6)","ORPLC_INFO":"쌀 : 국내산","CAL_INFO":"900 Kcal"}]}]})JSON";
  if (decodeMeal(good, date, *meal) && String(meal->menu) == "밥 / 국 (1.5.6)" && !meal->noMeal) ++passed;
  if (!decodeMeal(good, "20260923", *meal)) ++passed;
  String wrong = good; wrong.replace("7010806", "0000000");
  if (!decodeMeal(wrong, date, *meal)) ++passed;
  if (decodeMeal("{\"RESULT\":{\"CODE\":\"INFO-200\"}}", date, *meal) && meal->noMeal) ++passed;
  if (!decodeMeal("{\"RESULT\":{\"CODE\":\"ERROR-290\"}}", date, *meal)) ++passed;
  if (!decodeMeal("{broken", date, *meal)) ++passed;
  String longMenu; longMenu.reserve(2050);
  for (unsigned i = 0; i < 2048; ++i) longMenu += 'x';
  String oversized = good; oversized.replace("밥<br/>국 (1.5.6)", longMenu);
  if (!decodeMeal(oversized, date, *meal)) ++passed;
  wrong = good; wrong.replace("\"MMEAL_SC_CODE\":\"2\"", "\"MMEAL_SC_CODE\":\"3\"");
  if (!decodeMeal(wrong, date, *meal)) ++passed;
  delete meal;
  Serial.printf("[CHECK] parser %u/8 passed\n", passed);
  return passed == 8;
}

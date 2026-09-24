#pragma once
struct MealData {
  char date[9];
  char menu[2048];
  char origin[4096];
  char calories[96];
  bool noMeal;
};

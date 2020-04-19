#pragma once
#include <map>
#include <string>

struct PriceCheck {
  std::string name;
  std::string inner_html;
  std::string img_src;
  double min_suggest, max_suggest;
  bool is_watched;
};

class DataStore {
 public:
  static std::map<std::string, PriceCheck> items;

  static DataStore& getInstance() {
    static DataStore instance;
    return instance;
  }
  static bool AddItem(PriceCheck item);
  static void Clear();

 private:
  DataStore() {}
  DataStore(DataStore const&);
  void operator=(DataStore const&);
};
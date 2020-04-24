#ifndef DATASTORE_H_
#define DATASTORE_H_

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "include/base/cef_logging.h"
struct PriceCheck {
  std::string name;
  std::string img_src;
  double min_suggest, max_suggest;
  bool is_watched;
};

struct ItemDeal {
  std::string name;
  std::string location;
  std::string trader;
  double price;
  unsigned int mins_elapsed;
  unsigned int trade_id;
};

typedef std::vector<ItemDeal> ItemDealVec;

class DataStore {
 public:
  static DataStore* getInstance() {
    static DataStore instance;
    return &instance;
  }
  static bool AddItem(PriceCheck item);
  static bool RemoveItem(std::string& key);
  static PriceCheck GetItem(std::string& key);
  std::map<std::string, PriceCheck> GetItemsSnapshot();
  static void Clear();

 private:
  static std::map<std::string, PriceCheck> items;
  static std::recursive_mutex m_;
  DataStore(){};
  DataStore(DataStore const&);
  void operator=(DataStore const&);
};

#endif
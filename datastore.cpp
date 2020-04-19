#include "datastore.h"

std::map<std::string, PriceCheck> DataStore::items;

bool DataStore::AddItem(PriceCheck item) {
  if (items.find(item.name) == items.end()) {
    // does not exist
    items[item.name] = item;
    return false;
  } else {
    // exists
    items[item.name] = item;
    return true;
  }
}
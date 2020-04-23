#include "datastore.h"

// Instantiate static fields
std::map<std::string, PriceCheck> DataStore::items;
std::recursive_mutex DataStore::m_;
// Add an item to the datastore in an atomic fashion
bool DataStore::AddItem(PriceCheck item) {
  bool ret = false;

  m_.lock();

  if (items.find(item.name) == items.end()) {
    // does not exist
    items[item.name] = item;
    ret = false;
  } else {
    // exists
    items[item.name] = item;
    ret = true;
  }

  m_.unlock();

  return ret;
}

bool DataStore::RemoveItem(std::string& key) {
  size_t erased = 0;

  m_.lock();

  erased = items.erase(key);
  m_.unlock();

  return (erased > 0);
}

PriceCheck DataStore::GetItem(std::string& key) {
  m_.lock();
  return items[key];
  m_.unlock();
}

std::map<std::string, PriceCheck> DataStore::GetItemsSnapshot() {
  // LOG(INFO) << "LOCK " << std::this_thread::get_id();
  // m_.lock();
  std::map<std::string, PriceCheck> m(items);
  // m_.unlock();
  // LOG(INFO) << "UNLOCK " << std::this_thread::get_id();
  return m;
}
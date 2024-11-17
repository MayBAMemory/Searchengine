#include "../include/LRU.h"

LRUCache::LRUCache(int capacity) : _capacity(capacity) {}
int LRUCache::get(int key) {
  auto it = map.find(key);
  if (it == map.end()) {
    return -1;
  } else {
    cache.splice(cache.begin(), cache, it->second);
    return it->second->second;
  }
}

void LRUCache::put(int key, int value) {
  auto it = map.find(key);
  if (it != map.end()) {
    it->second->second = value;
    cache.splice(cache.begin(), cache, it->second);
  } else {
    if (cache.size() == _capacity) {
      int old_node = cache.back().first;
      cache.pop_back();
      map.erase(old_node);
    }
    cache.emplace_front(key, value);
    map[key] = cache.begin();
  }
}

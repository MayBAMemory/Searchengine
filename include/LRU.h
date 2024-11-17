#include <list>
#include <unordered_map>
using std::list;
using std::pair;
using std::unordered_map;

class LRUCache {
private:
  int _capacity;
  list<pair<int, int>> cache;
  unordered_map<int, list<pair<int, int>>::iterator> map;

public:
  LRUCache(int capacity);
  int get(int key);
  void put(int key, int value);
};

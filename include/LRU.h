#include "nlohmann/json.hpp"
#include "sw/redis++/redis++.h"
#include <iostream>
#include <list>
#include <unordered_map>
using std::list;
using std::pair;
using std::string;
using std::unordered_map;
using namespace nlohmann;
using namespace sw::redis;
struct Article {
  Article() = default;
  Article(const std::string &t, const std::string &c) : title(t), content(c) {}
  Article(pair<string, string> p) : title(p.first), content(p.second) {}
  string title;
  string content;
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Article, title, content)

  string serialize() const { return json(*this).dump(); }
  static Article deserialize(const string &str) {
    json j = json::parse(str);
    return j.get<Article>();
  }
};

class LRU_Cache {
private:
  int _capacity;
  list<pair<int, Article>> cache;
  unordered_map<int, list<pair<int, Article>>::iterator> map;
  const string REDIS_PREFIX = "articles_cache";
  Redis* redis_client;

public:
  LRU_Cache(int capacity, Redis& redis);
  std::optional<Article> get(int key);
  void put(int key, const Article& article);
  void clear();
};

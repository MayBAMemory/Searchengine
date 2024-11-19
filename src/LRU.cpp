#include "../include/LRU.h"

LRU_Cache::LRU_Cache(int capacity, Redis &redis)
    : _capacity(capacity), redis_client(&redis) {}

std::optional<Article> LRU_Cache::get(int key) {
  auto it = map.find(key);
  if (it == map.end()) {
    try {
      auto redis_key = REDIS_PREFIX + std::to_string(key);
      auto article_str = redis_client->get(redis_key);

      if (article_str) { // 缓存没找着,存入redis
        Article article = Article::deserialize(*article_str);
        put(key, article);
        return article;
      }
      return std::nullopt;
    } catch (const Error &e) {
      // redis有错误
      return std::nullopt;
    }
  } else {
    // 缓存存在，it指向该缓存，将cache中的it->second移动到开头，也就是LRU的逻辑
    cache.splice(cache.begin(), cache, it->second);
    return it->second->second;
  }
}

void LRU_Cache::put(int key, const Article &article) {
  // std::cout << "当前存储缓存数量：" << cache.size() << "\n";
  auto it = map.find(key);
  if (it != map.end()) {
    // 有该元素，那么只将节点往开头放,并赋予新值
    it->second->second = article;
    cache.splice(cache.begin(), cache, it->second);
  } else {
    if (cache.size() >= _capacity) {
      std::cout << "删除了一份redis数据" << "\n";
      int old_node = cache.back().first;
      cache.pop_back();
      map.erase(old_node);

      // 超过缓存上限，删除redis上的数据
      try {
        redis_client->del(REDIS_PREFIX + std::to_string(old_node));
      } catch (const Error &e) {
      }
    }
    // 没该元素，那么在头部创建节点
    cache.emplace_front(key, article);
    map[key] = cache.begin();
  }

  try {
    auto redis_key = REDIS_PREFIX + std::to_string(key);
    redis_client->set(redis_key, article.serialize());
  } catch (const Error &e) {
  }
}

void LRU_Cache::clear() {
  try {
    std::cout<<"清理缓存中"<<"\n";
    redis_client->flushdb();

  } catch (const Error &e) {
  }
}

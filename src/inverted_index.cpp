#include "../include/inverted_index.h"

void InvertedIndex::storeDocument() {
  Redis redis(_redisUrl);
  for (auto const &[idx, term_weight] : _w) {
    size_t doc_id = idx;
    for (const auto &[term, weight] : term_weight) { 
      // 建立倒排索引库
      // 在倒排索引中添加文档ID
      // key: term:{term}
      string weight_str = to_string(weight);
      redis.zadd(term, to_string(doc_id), weight);

      // 建立权重库
      // 在文档的hash中存储词项权重
      // redis.hset(doc_key, term, weight_str);
    }
  }
}

// 查询包含特定词项的文档ID，按权重降序排序
void InvertedIndex::queryTerm(const string &term,vector<size_t>& docIds) {
  Redis redis(_redisUrl);
  string term_key = "term:" + term;
  vector<pair<string, double>> results;
  // 使用 ZREVRANGE 获取权重最高的文档ID
  redis.zrevrangebyscore(term_key, UnboundedInterval<double>{},
                         back_inserter(results));
  for (const auto &[docId, w] : results) {
    docIds.push_back(stoull(docId));
  }
}

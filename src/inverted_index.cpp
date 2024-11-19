#include "../include/inverted_index.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
using std::ifstream;
using std::istringstream;
using std::ofstream;

vector<double> buildBase(const vector<string> &words) {
  unordered_map<string, size_t> userTF;
  for (auto const &word : words) {
    userTF[word]++;
  }
  unordered_map<string, double> IDF;
  ifstream iIdf("data/IDF.txt");
  string line, word;
  double IDFNum;
  while (getline(iIdf, line)) {
    istringstream iss(line);
    if (iss >> word >> IDFNum) {
      IDF[word] = IDFNum;
    }
  }
  //
  // for (auto const &[word, IDFNum] : IDF) {
  //   norm_factor += IDFNum * IDFNum;
  // }
  //
  double norm_factor = 0.0;
  for (auto const &[word, freq] : userTF) {
    if (IDF.find(word) != IDF.end()) {
      double temp = IDF[word] * static_cast<double>(freq);
      norm_factor += temp * temp;
    }
  }
  double normedFactor = sqrt(norm_factor);
  vector<double> Base;
  for (auto const &[word, freq] : userTF) {
    // cout<<"外面的循环：word: "<<word<<" , freq:"<<freq<<"\n";
    if (IDF.find(word) != IDF.end()) {
      // cout<<"freq:"<<freq<<"\n";
      Base.emplace_back((IDF[word] * freq) / normedFactor);
    }
  }
  return Base;
}

unordered_map<string, vector<pair<size_t, double>>>
InvertedIndex::loadInvertIndex(const std::string &filename) {
  unordered_map<string, vector<pair<size_t, double>>> index;
  ifstream in(filename);
  string word;
  size_t docId;
  double weight;

  while (in >> word >> docId >> weight) {
    _hashInvertIndex[word][docId] = weight;
    index[word].emplace_back(docId, weight);
  }

  // 对每个词的文档列表按文档ID排序，方便后续求交集
  for (auto &entry : index) {
    std::sort(entry.second.begin(), entry.second.end());
  }

  return index;
}
unordered_map<string, unordered_map<size_t, double>>
InvertedIndex::getHashInvertIndex() {
  return _hashInvertIndex;
}

void InvertedIndex::storeInvertIndex(
    unordered_map<size_t, unordered_map<string, double>> w) {
  ofstream wr("data/invertIndex.dat");
  for (auto const &doc : w) {
    for (const auto &mapp : doc.second) {
      wr << mapp.first << " ";
      wr << doc.first << " " << mapp.second << "\n";
    }
  }
}

void
processQuery(const vector<string> &terms,
             unordered_map<string, vector<pair<size_t, double>>> index,
             unordered_map<string, unordered_map<size_t, double>> normalized_w,
             vector<pair<size_t, double>> &docIds, size_t top_k) {
  if (terms.empty())
    return;
  // if (terms.size() == 1) {
  //   auto results = index[terms[0]];
  //   // 按权重排序
  //   std::sort(results.begin(), results.end(),
  //             [](const auto &a, const auto &b) { return a.second > b.second; });
  //   if (results.size() > top_k) {
  //     results.resize(top_k);
  //   }
  //   return;
  // }

  // 获取所有词的文档列表
  vector<vector<pair<size_t, double>>> docs_lists;
  for (const auto &term : terms) {
    if (index.count(term)) {
      docs_lists.push_back(index[term]);
    }
  }

  if (docs_lists.empty())
    return;

  // 使用优先队列存储最终结果
  std::priority_queue<pair<double, size_t>, vector<pair<double, size_t>>,
                      std::less<pair<double, size_t>>> pq;

  // 使用多路归并方式求交集
  vector<size_t> positions(docs_lists.size(), 0);

  while (true) {
    // 找到当前最小的文档ID
    size_t min_doc_id = SIZE_MAX;
    for (size_t i = 0; i < docs_lists.size(); i++) {
      if (positions[i] < docs_lists[i].size() &&
          docs_lists[i][positions[i]].first < min_doc_id) {
        min_doc_id = docs_lists[i][positions[i]].first;
      }
    }

    if (min_doc_id == SIZE_MAX)
      break;

    // 检查是否所有列表都包含该文档
    bool all_contain = true;
    double total_weight = 0;

    for (size_t i = 0; i < docs_lists.size(); i++) {
      if (positions[i] >= docs_lists[i].size() ||
          docs_lists[i][positions[i]].first != min_doc_id) {
        all_contain = false;
        break;
      }
      total_weight += docs_lists[i][positions[i]].second;
    }

    // 如果所有列表都包含该文档，加入结果集
    if (all_contain) {
      pq.push({total_weight, min_doc_id});
      if (pq.size() > top_k) {
        pq.pop();
      }
    }

    // 更新位置
    for (size_t i = 0; i < docs_lists.size(); i++) {
      if (positions[i] < docs_lists[i].size() &&
          docs_lists[i][positions[i]].first == min_doc_id) {
        positions[i]++;
      }
    }
  }

  // 转换优先队列到vector并按权重降序排序
  vector<pair<string, double>> results;
  while (!pq.empty()) {
    results.emplace_back(to_string(pq.top().second), pq.top().first);
    pq.pop();
  }
  std::reverse(results.begin(), results.end());
  // 算X
  vector<double> query_vector = buildBase(terms);
  // 算Y
  vector<pair<string, double>> doc_scores;
  for (const auto &[doc_id, w] : results) {
    // Y
    vector<double> doc_vector;
    for (const auto &term : terms) {
      double weight = normalized_w[term][stoul(doc_id)];
      doc_vector.push_back(weight);
    }
    for (auto const &w : doc_vector) {
      // cout<<"doc_vector: "<<w<<"\n";
    }
    // stable_sort(doc_vector.begin(),doc_vector.end(),[](double a,double b){
    //   return a>b;
    // });
    // 计算余弦相似度
    double numerator = 0.0;  // 分子
    double query_norm = 0.0; // 查询向量的模
    double doc_norm = 0.0;   // 文档向量的模

    for (size_t i = 0; i < terms.size(); i++) {
      numerator += query_vector[i] * doc_vector[i];
      query_norm += query_vector[i] * query_vector[i];
      doc_norm += doc_vector[i] * doc_vector[i];
    }

    double similarity = numerator / (sqrt(query_norm) * sqrt(doc_norm));
    doc_scores.emplace_back(doc_id, similarity);
  }
                                               
  std::cout<<"doc_socres："<<doc_scores.size()<<"\n";
  sort(doc_scores.begin(), doc_scores.end(),
       [](const pair<string, double> &a, const pair<string, double> &b) {
         return a.second > b.second;
       });

  size_t docNums = 10;
  for (size_t i = 0; i < fmin(results.size(), docNums); ++i) {
    docIds.emplace_back(
        pair(stoull(doc_scores[i].first), doc_scores[i].second));
    // cout << "DocID: " << doc_scores[i].first
    //      << ", 相似度: " << doc_scores[i].second << "\n";
  }

}

// void saveIndex(
//     const std::unordered_map<size_t, std::unordered_map<std::string, double>>
//         &index,
//     const std::string &filename) {
//   std::ofstream out(filename, std::ios::binary);
//
//   // 写入外层map的大小
//   size_t outerSize = index.size();
//   out.write(reinterpret_cast<const char *>(&outerSize), sizeof(outerSize));
//
//   // 遍历外层map
//   for (const auto &[key, innerMap] : index) {
//     // 写入key
//     out.write(reinterpret_cast<const char *>(&key), sizeof(key));
//
//     // 写入内层map的大小
//     size_t innerSize = innerMap.size();
//     out.write(reinterpret_cast<const char *>(&innerSize), sizeof(innerSize));
//
//     // 遍历内层map
//     for (const auto &[str, value] : innerMap) {
//       // 写入字符串长度
//       size_t strLen = str.length();
//       out.write(reinterpret_cast<const char *>(&strLen), sizeof(strLen));
//
//       // 写入字符串内容
//       out.write(str.c_str(), strLen);
//
//       // 写入double值
//       out.write(reinterpret_cast<const char *>(&value), sizeof(value));
//     }
//   }
//   out.close();
// }
//
// // 读取函数
// std::unordered_map<size_t, std::unordered_map<std::string, double>>
// loadIndex(const std::string &filename) {
//   std::unordered_map<size_t, std::unordered_map<std::string, double>> index;
//   std::ifstream in(filename, std::ios::binary);
//
//   // 读取外层map的大小
//   size_t outerSize;
//   in.read(reinterpret_cast<char *>(&outerSize), sizeof(outerSize));
//
//   // 读取所有元素
//   for (size_t i = 0; i < outerSize; i++) {
//     // 读取key
//     size_t key;
//     in.read(reinterpret_cast<char *>(&key), sizeof(key));
//
//     // 读取内层map的大小
//     size_t innerSize;
//     in.read(reinterpret_cast<char *>(&innerSize), sizeof(innerSize));
//
//     // 读取内层map的所有元素
//     std::unordered_map<std::string, double> innerMap;
//     for (size_t j = 0; j < innerSize; j++) {
//       // 读取字符串长度
//       size_t strLen;
//       in.read(reinterpret_cast<char *>(&strLen), sizeof(strLen));
//
//       // 读取字符串
//       std::string str(strLen, '\0');
//       in.read(&str[0], strLen);
//
//       // 读取double值
//       double value;
//       in.read(reinterpret_cast<char *>(&value), sizeof(value));
//
//       innerMap[str] = value;
//     }
//
//     index[key] = std::move(innerMap);
//   }
//
//   in.close();
//   return index;
// }
//
// void InvertedIndex::storeDocument() {
//   Redis redis(_redisUrl);
//   for (auto const &[idx, term_weight] : _w) {
//     size_t doc_id = idx;
//     for (const auto &[term, weight] : term_weight) {
//       // 建立倒排索引库
//       // 在倒排索引中添加文档ID
//       // key: term:{term}
//       string weight_str = to_string(weight);
//       redis.zadd(term, to_string(doc_id), weight);

// 建立权重库
// 在文档的hash中存储词项权重
// redis.hset(doc_key, term, weight_str);
//     }
//   }
// }

// 查询包含特定词项的文档ID，按权重降序排序
// void InvertedIndex::queryTerm(const string &term, vector<size_t> &docIds) {
//   Redis redis(_redisUrl);
//   string term_key = "term:" + term;
//   vector<pair<string, double>> results;
//   // 使用 ZREVRANGE 获取权重最高的文档ID
//   redis.zrevrangebyscore(term_key, UnboundedInterval<double>{},
//                          back_inserter(results));
//   for (const auto &[docId, w] : results) {
//     docIds.push_back(stoull(docId));
//   }
// }

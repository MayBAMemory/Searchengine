#include "../include/recommandWords.h"
#include "../include/utf8.h"
#include <fstream>
#include <sstream>

;
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

// void loadInvertedIndex(
//     Redis &redis, unordered_map<size_t, unordered_map<string, double>> _w_new) {
//   vector<string> terms; // 存储所有term的key
//
//   // 获取所有term
//   redis.keys("*", back_inserter(terms));
//
//   for (const auto &term : terms) {
//     // 获取term对应的文档ID和权重
//     vector<pair<string, double>> doc_weights;
//     redis.zrangebyscore(term, UnboundedInterval<double>{},
//                         back_inserter(doc_weights)); // 获取Sorted Set内容
//
//     for (const auto &[doc_id_str, weight] : doc_weights) {
//       size_t doc_id = stoi(doc_id_str); // 将doc_id从字符串转换为整数
//       _w_new[doc_id][term] = weight;    // 构建哈希表
//     }
//   }
// }

void searchWords(Redis &redis, const vector<string> &terms,
                   unordered_map<size_t, unordered_map<string, double>> &normalized_w,
                 vector<pair<size_t,double>> &DocId) {
  vector<string> tempTerms;
  string destKey = "temp:search";
  cout << "客户端搜索：";
  for (auto const &term : terms) {
    string tempTerm = "temp:" + term;

    cout << term << " ";
    tempTerms.emplace_back(tempTerm);
    // 词为单位存入临时key
    redis.zunionstore(tempTerm, {term});
  }
  cout << "\n";
  redis.zinterstore(destKey, tempTerms.begin(), tempTerms.end());
  vector<pair<string, double>> results;
  redis.zrevrangebyscore(destKey, UnboundedInterval<double>{},
                         back_inserter(results));

  // 删除临时键
  for (const auto &key : tempTerms) {
    redis.del(key);
  }
  redis.del(destKey);
  if (results.empty()) {
    cout << "没找到" << endl;
    return;
  }
  //----------

  // 算X
  vector<double> query_vector = buildBase(terms);
  // 算Y
  vector<pair<string, double>> doc_scores;
  for (const auto &[doc_id, w] : results) {
    // Y
    vector<double> doc_vector;
    for (const auto &term : terms) {
      double weight = normalized_w[stoul(doc_id)][term];
      doc_vector.push_back(weight);
    }
    for(auto const&w:doc_vector){
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

  sort(doc_scores.begin(), doc_scores.end(),
       [](const pair<string, double> &a, const pair<string, double> &b) {
         return a.second > b.second;
       });

  size_t docNums = 10;
  for (size_t i = 0; i < min(results.size(), docNums); ++i) {
    DocId.emplace_back(pair(stoull(doc_scores[i].first),doc_scores[i].second));
    // cout << "DocID: " << doc_scores[i].first
    //      << ", 相似度: " << doc_scores[i].second << "\n";
  }
}

void recommandWords(Redis &redis, const string word, vector<string> &recWords) {
  vector<string> tempKeys;
  string destKey = "temp:result:" + word;
  auto it = word.begin();
  while (it != word.end()) {
    // 读取一个完整字,it会跳到下个字
    uint32_t codePoint = utf8::next(it, word.end());
    string char_utf8;
    // codepoint转回utf8字符串
    utf8::append(codePoint, back_inserter(char_utf8));
    string tempKey = "temp:" + char_utf8;
    tempKeys.push_back(tempKey);

    // 复制原始集合到临时集合
    // 具体说，将一个字的<string,size_t>键值对复制给了一个临时的键值对
    // 比如word为你好
    // 循环后redis会存储
    // <temp:你,<包含你的词, 对应词频>>
    // <temp:好,<包含好的词，对应词频>>
    redis.zunionstore(tempKey, {char_utf8});
  }

  // 将你和好合并进destKey
  redis.zunionstore(destKey, tempKeys.begin(), tempKeys.end());
  vector<pair<string, double>> results;
  // 将<包含x的词，x对应词频>尽数存进results，在外面去限定输出数量
  //  且已按词频排序
  redis.zrevrangebyscore(destKey, UnboundedInterval<double>{},
                         back_inserter(results));

  for (const auto &key : tempKeys) {
    redis.del(key);
  }
  redis.del(destKey);

  if (results.empty()) {
    cout << "没找到" << endl;
    return;
  }

  // 加上最小编辑距离信息
  vector<WordInfo> processed_results;
  for (const auto &result : results) {
    int dist = editDistance(word, result.first);
    processed_results.emplace_back(result.first, result.second, dist);
  }

  sort(processed_results.begin(), processed_results.end(),
       [](const WordInfo &a, const WordInfo &b) {
         if (a.distance != b.distance) {
           return a.distance < b.distance;
         }
         return a.freq > b.freq;
       });

  for (size_t i = 0; i < min(size_t(10), processed_results.size()); ++i) {
    const auto &result = processed_results[i];
    recWords.emplace_back(result.word);
    cout << "单词：" << result.word << " ，词频：" << result.freq
         << " ，编辑距离：" << result.distance << endl;
  }
}

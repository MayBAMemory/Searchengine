#include "../include/recommandWords.h"
#include "../include/utf8.h"

;
void buildBase(unordered_map<string, size_t>& userTF, vector<size_t> &DocId) {
  for()
}

void searchWords(Redis &redis, const vector<string> &terms,
                 vector<size_t> &DocId) {
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
  for (const auto &key : tempTerms) {
    redis.del(key);
  }
  redis.del(destKey);
  if (results.empty()) {
    cout << "没找到" << endl;
    return;
  }

  size_t docNums = 10;
  for (size_t i = 0; i < min(results.size(), docNums); ++i) {
    // DocId.emplace_back(stoull(results[i].first));
    cout << results[i].first << "\n";
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

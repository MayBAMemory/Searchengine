#include "../include/INIReader.h"
#include "../include/simhash/cppjieba/Jieba.hpp"
#include <algorithm>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <sw/redis++/redis++.h>
#include <vector>
using namespace std;
using namespace sw::redis;

class DictProducer {
public:
  DictProducer();
  DictProducer(const string &_cnStopsDir);

  void buildEnDict(const string& dictDir);
  // 往redis存英文索引
  void load_dict_and_store_to_redis(const string& dicDir, Redis &redis);
  void buildCnDict(const string &cnYuliao, const string &dictDir,
                   const cppjieba::Jieba &jieba);
  void buildCnDictFromString(string str, size_t idx,unordered_set<string> &set, unordered_map<size_t, unordered_map<string, size_t>> &TF, const cppjieba::Jieba &jieba);
  // 往redis存中文索引
  void save_dict_to_redis(Redis &redis);
  const string _enYuliaoDir;
  const string _enStopsDir;
  const string _cnYuliaoDir;
  const string _cnStopsDir;
  const string _dictDir;
  const string _dictDire;

private:
  // 英文部分
  void clean(ifstream &file, ifstream &stopFile);
  void read();
  void storeDict(const string& dictDir);
  void store_to_redis(Redis &redis, const string &word, int freq);
  // 中文
  // 结巴切词并统计词频
  void cut_and_count_words(const string& cnYuliao,const cppjieba::Jieba &jieba);
  // 加载停止词到一个set
  set<string> load_stopwords();
  // 转为宽字符后正则判断
  bool is_chinese(const std::string &word);
  void clean_word_freq(map<string, int> &word_freq);
  void store_word_with_frequency(Redis &redis, const string &word, int freq);
  void save_dict_to_dat(const string& dictDir);

private:
  map<string, int> _dict;
  map<string, int> _cnFreqDict;
};

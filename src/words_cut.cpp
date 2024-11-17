#include "../include/words_cut.h"
INIReader _reader("conf/myconf.conf");

void DictProducer::store_to_redis(Redis &redis, const string &word, int freq) {
  set<char> unique_chars(word.begin(), word.end());
  for (const char &ch : unique_chars) {
    string key(1, ch);
    redis.zadd(key, word, freq);
  }
}

void DictProducer::load_dict_and_store_to_redis(const string &dictDir,
                                                Redis &redis) {
  ifstream file(dictDir);
  string line, word;
  int freq;
  while (getline(file, line)) {
    istringstream iss(line);
    if (iss >> word >> freq) {
      store_to_redis(redis, word, freq);
    }
  }
}

void DictProducer::clean(ifstream &file, ifstream &stopFile) {
  string line, word, stopWord;
  set<string> stops;
  // 获取停止词到set
  while (getline(stopFile, stopWord)) {
    if (!stopWord.empty()) {
      if (stopWord.back() == '\r') {
        stopWord.pop_back();
      }
      stops.insert(stopWord);
    }
  } // 大清洗
  while (getline(file, line)) {
    istringstream iss(line);
    string rawWord;
    while (iss >> rawWord) {
      // 转换为小写
      transform(rawWord.begin(), rawWord.end(), rawWord.begin(), ::tolower);
      word.clear();
      for (char c : rawWord) {
        if (isalpha(c)) {
          word += c;
        }
      }
      if (word.empty() || stops.find(word) != stops.end()) {
        continue;
      }
      _dict[word]++;
    }
  }
}

void DictProducer::read() {
  ifstream file(_enYuliaoDir);
  ifstream stopFile(_enStopsDir);
  if (!file.is_open() || !stopFile.is_open()) {
    cerr << "opening file failed: " << _enYuliaoDir << " or " << _enStopsDir
         << endl;
    return;
  }
  // 清洗后才存入_dict
  clean(file, stopFile);
  file.close();
}

void DictProducer::storeDict(const string &dictDir) {
  ofstream file(dictDir, std::ios::app);
  for (const auto &[f, s] : _dict) {
    file << f << " " << s << "\n";
  }
  file.close();
}

void DictProducer::buildEnDict(const string &dictDir) {
  read();
  storeDict(dictDir);
}

//=================
// 结巴切词并统计词频
void DictProducer::cut_and_count_words(const string &cnYuliao,
                                       const cppjieba::Jieba &jieba) {
  ifstream file(cnYuliao);
  string line;
  vector<string> words;
  while (getline(file, line)) {
    jieba.Cut(line, words, true); // 使用Jieba分词
    for (const auto &word : words) {
      _cnFreqDict[word]++;
    }
  }
  file.close();
  // 存完就清洗
  clean_word_freq(_cnFreqDict);
}

// 加载停用词到stopwords
set<string> DictProducer::load_stopwords() {
  set<string> stopwords;
  ifstream file(_cnStopsDir);
  string word;
  if (!file.is_open()) {
    std::cerr << "无法打开停用词文件: " << _cnStopsDir << std::endl;
    return stopwords;
  }
  while (getline(file, word)) {
    // 移除可能存在的回车符或空格
    if (!word.empty()) {
      if (word.back() == '\r') {
        word.pop_back();
      }
      stopwords.insert(word);
    }
  }
  file.close();
  return stopwords;
}

bool DictProducer::is_chinese(const std::string &word) {
  wregex chinese_wregex(L"^[\u4e00-\u9fa5]+$");
  // 转换为宽字符再判断
  wstring_convert<codecvt_utf8<wchar_t>> converter;
  wstring wword = converter.from_bytes(word);
  return regex_match(wword, chinese_wregex);
}

void DictProducer::clean_word_freq(map<string, int> &word_freq) {
  auto stopwords = load_stopwords();
  for (auto it = word_freq.begin(); it != word_freq.end();) {
    if (!is_chinese(it->first) ||
        stopwords.find(it->first) != stopwords.end()) {
      it = word_freq.erase(it);
    } else {
      ++it;
    }
  }
}

void DictProducer::store_word_with_frequency(Redis &redis, const string &word,
                                             int freq) {
  wstring_convert<codecvt_utf8<wchar_t>> converter;
  set<string> unique_chars;
  for (const char &ch : word) {
    // 转换为宽字符
    wstring wword = converter.from_bytes(word);
    for (const wchar_t &wch : wword) {
      // 遍历一整个中文字，再转回string
      unique_chars.insert(converter.to_bytes(wch)); // 将每个汉字作为键
    }
  }
  for (const auto &ch : unique_chars) {
    wstring wword = converter.from_bytes(word);
    for (const wchar_t &wch : wword) {
      redis.zadd(converter.to_bytes(wch), word, freq); // 将词语和词频以zset存入
    }
  }
}

void DictProducer::save_dict_to_redis(Redis &redis) {
  for (const auto &[word, freq] : _cnFreqDict) {
    store_word_with_frequency(redis, word, freq);
  }
}

void DictProducer::save_dict_to_dat(const string &dictDir) {
  ofstream file(dictDir, std::ios::app);
  for (const auto &[word, freq] : _cnFreqDict) {
    file << word << " " << freq << "\n";
  }
}

void DictProducer::buildCnDict(const string &cnYuliao, const string &dictDir,
                               const cppjieba::Jieba &jieba) {
  cut_and_count_words(cnYuliao, jieba);
  save_dict_to_dat(dictDir);
}

void DictProducer::buildCnDictFromString(
  string str, size_t idx,unordered_set<string> &set,
    unordered_map<size_t, unordered_map<string, size_t>> &TF,
    const cppjieba::Jieba &jieba) {
  vector<string> words;
  jieba.Cut(str, words, true); // 使用Jieba分词
  for (const auto &word : words) {
    _cnFreqDict[word]++;
  }
  clean_word_freq(_cnFreqDict);
  for (const auto &[word, freq] : _cnFreqDict) {
    TF[idx][word] = freq; // 将单词及其频率添加到TF中
    set.insert(word);
  }
  _cnFreqDict.clear();
}

DictProducer::DictProducer()
    : _enYuliaoDir(_reader.Get("user", "EnglishYuliao", "UNKNOWN")),
      _enStopsDir(_reader.Get("user", "EnglishStop", "UNKNOWN")),
      _cnYuliaoDir(_reader.Get("user", "ChineseYuliao", "UNKNOWN")),
      _cnStopsDir(_reader.Get("user", "ChineseStop", "UNKNOWN")),
      _dictDir(_reader.Get("user", "dict", "UNKNOWN")) {}

DictProducer::DictProducer(const string &cnStops)
    : _enYuliaoDir(_reader.Get("user", "EnglishYuliao", "UNKNOWN")),
      _enStopsDir(_reader.Get("user", "EnglishStop", "UNKNOWN")),
      _cnYuliaoDir(_reader.Get("user", "ChineseYuliao", "UNKNOWN")),
      _cnStopsDir(cnStops),
      _dictDir(_reader.Get("user", "dict", "UNKNOWN")) {}

// int main() {
//
//   // 初始化英文词典类
//   DictProducer dict;
//   // 初始化Redis
//   Redis redis(
//       reader.Get("user", "redisServer", "UNKNOWN")); // 连接到本地Redis服务器
//   // 初始化Jieba
//   cppjieba::Jieba jieba(reader.Get("user", "dictPath", "UNKNOWN"),
//                         reader.Get("user", "modelPath", "UNKNOWN"),
//                         reader.Get("user", "user_dict_path", "UNKNOWN"),
//                         reader.Get("user", "idfPath", "UNKNOWN"),
//                         reader.Get("user", "stopWords", "UNKNOWN"));
//
//   // 存入英文词频dat到本地
//   dict.buildEnDict("");
//   // dict.load_dict_and_store_to_redis(redis);
//
//   // 存入中文词频dat到本地
//   dict.buildCnDict(dict._cnYuliaoDir, dict._dictDire, jieba);
//   // dict.save_dict_to_redis(redis);
// }

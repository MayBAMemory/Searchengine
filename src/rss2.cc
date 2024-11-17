#include "../include/rss2.h"
#include "../include/INIReader.h"
#include "../include/article_manager.h"
#include "../include/inverted_index.h"
#include "../include/tinyxml2.h"
#include "../include/words_cut.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

using std::endl;
using std::ofstream;
using std::string;
using std::vector;
using namespace tinyxml2;

#define textSize 75

unordered_map<size_t, unordered_map<string, size_t>> _TF;
unordered_map<string, int> DF;
unordered_map<string, double> IDF;
unordered_map<size_t, unordered_map<string, double>> w;
unordered_map<size_t, double> norm_factors;
unordered_map<size_t, unordered_map<string, double>> normalized_w;

INIReader reader("conf/myconf.conf");

DictProducer dict(reader.Get("user", "ChineseStop", "UNKNOWN"));
// 初始化Jieba
cppjieba::Jieba jieba(reader.Get("user", "dictPath", "UNKNOWN"),
                      reader.Get("user", "modelPath", "UNKNOWN"),
                      reader.Get("user", "user_dict_path", "UNKNOWN"),
                      reader.Get("user", "idfPath", "UNKNOWN"),
                      reader.Get("user", "stopWords", "UNKNOWN"));

size_t idxG = 1;
size_t offS = 0;

struct RSSIteam {
  string _title;
  string _link;
  string _description;
};

class RSS {
public:
  RSS(size_t capa) { _rss.reserve(capa); }

  // 读文件
  void read(const string &filename) {
    XMLDocument doc;
    doc.LoadFile(filename.c_str());
    if (doc.ErrorID()) {
      std::cerr << "loadFile fail" << endl;
      std::cerr << doc.ErrorIDToName(doc.ErrorID()) << endl;
      return;
    }

    XMLElement *itemNode = doc.FirstChildElement("rss")
                               ->FirstChildElement("channel")
                               ->FirstChildElement("item");
    while (itemNode) {
      string title = itemNode->FirstChildElement("title")->GetText();
      string link = itemNode->FirstChildElement("link")->GetText();
      string description;
      if (itemNode->FirstChildElement("description")) {
        description = itemNode->FirstChildElement("description")->GetText();
      } else if (itemNode->FirstChildElement("content")) {
        description = itemNode->FirstChildElement("content")->GetText();
      } else {
        description = title;
      }
      std::regex reg("<[^>]+>"); // 通用正则表达式
      description = regex_replace(description, reg, "");

      RSSIteam rssItem;

      rssItem._title = title;
      rssItem._link = link;
      rssItem._description = description;

      _rss.push_back(rssItem);

      itemNode = itemNode->NextSiblingElement("item");
    }
  }

  // 写文件
  void store(const string &filename, const string &offset, ArticleManager &man,
             const string &pageContents) {
    // 网页库
    ofstream ofs(filename, std::ios::app);
    // 偏移量库
    ofstream ofsO(offset, std::ios::app);
    // 网页内容库
    ofstream ofsC(pageContents, std::ios::app);
    if (!ofs || !ofsO) {
      std::cerr << "open " << filename << " fail!" << endl;
      return;
    }

    for (size_t idx = 0; idx != _rss.size(); ++idx) {
      if (!man.addArticle(idxG, _rss[idx]._description)) {
        // cout<<"找到搬运党了！\n";
      } else {
        // 建立临时容器存储每篇文章所含词语，set自动去重，每次循环存入全局DF
        unordered_set<string> temp_set;

        // 存储网页库
        ofs << "<doc>\n\t"
            << "<id>" << idxG << "</id>\n\t"
            << "<url>" << _rss[idx]._link << "</url>\n\t"
            << "<title>" << _rss[idx]._title << "</title>\n\t"
            << "<content>" << _rss[idx]._description << "</content>\n"
            << "</doc>";
        ofs << '\n';

        // 存储一个包含所有文章内容的文章库
        ofsC << _rss[idx]._description << " ";

        // 传入该函数以计算TF与DF
        dict.buildCnDictFromString(_rss[idx]._description, idxG, temp_set, _TF,
                                   jieba);
        // 将set存入DF
        for (auto const &word : temp_set) {
          DF[word]++;
        }

        // 计算偏移量
        // 每篇文章的长度，textsize为固定的标签长度
        size_t rssSize = 0;
        rssSize += (strlen(std::to_string(idxG).c_str()) +
                    strlen(_rss[idx]._link.c_str()) +
                    strlen(_rss[idx]._title.c_str()) +
                    strlen(_rss[idx]._description.c_str()) + textSize);
        // 存储偏移量库
        ofsO << idxG << " " << offS << " " << rssSize << "\n";
        // 下一篇文章偏移量
        offS += (rssSize);
        ++idxG;
      }
    }
    ofs.close();
    ofsO.close();
  }

  void storeIDF() {
    ofstream oIdf("data/IDF.txt");
    for (auto const [word, IDFnum] : IDF) {
      oIdf << word << " " << IDFnum <<"\n";
    }
    oIdf.close();
  }

  void copyIDF(unordered_map<string, double> &destIDF) {
    for (auto const [word, IDFnum] : IDF) {
      destIDF[word] = IDFnum;
    }
  }

  void storeInvertIndextoRedis() {
    InvertedIndex index(w, reader.Get("user", "redisInvertIndex", "UNKNOWN"));
    cout << index._redisUrl << endl;

    index.storeDocument();
  }

  // void storeTFDF() {
  //   ofstream st("data/TF.txt");
  //   ofstream sd("data/DF.txt");
  //   // 遍历输出TF
  //   for (const auto &map : _TF) {
  //     st << "文章编号：" << map.first << ":\n";
  //     for (const auto &mapp : map.second) {
  //       st << mapp.first << " " << mapp.second << "\n";
  //     }
  //   }
  //   // 遍历输出DF
  //   for (const auto &map : DF) {
  //     sd << map.first << " " << map.second << "\n";
  //   }
  // }
  void buildInvertIndex() {
    calcIDF();
    calcW();
    calcWResult();
  }

private:
  // log2(N/(DF+1))
  void calcIDF() {
    // 遍历计算IDF
    for (auto const &map : DF) {
      IDF.insert({map.first, log2(static_cast<double>(idxG) /
                                  static_cast<double>((map.second) + 1))});
    }
  }

  void calcW() {
    for (auto const &doc : _TF) {
      size_t docID = doc.first;
      auto &terms = doc.second;
      for (auto const &term : terms) {
        string word = term.first;
        size_t freq = term.second;
        if (IDF.find(word) != IDF.end()) {
          double tfIdf = static_cast<double>(freq) * IDF[word];
          w[docID][word] = tfIdf;
        }
      }
    }
  }

  void calcWResult() {
    // 计算sqrt(w1^2 + w2^2 +...+ wn^2)
    for (auto const &doc : w) {
      size_t docid = doc.first;
      auto &terms = doc.second;
      double sum_of_squares = 0.0;
      for (auto const &term : terms) {
        double tfidf = term.second;
        sum_of_squares += (tfidf * tfidf);
      }
      double norm_factor = sqrt(sum_of_squares);
      norm_factors[docid] = norm_factor;
      // norm_factors存储文章id，与文章内词语权重平方和的平方根
    }

    for (auto const &doc : w) {
      size_t docid = doc.first;
      auto &terms = doc.second;
      double norm_factor = norm_factors[docid];
      for (auto const &term : terms) {
        string word = term.first;
        double tfIdf = term.second;

        double normalized_tfIdf = tfIdf / norm_factor;
        normalized_w[docid][word] = normalized_tfIdf;
      }
    }
    ofstream wr("data/invertIndex.dat");
    for (auto const &doc : normalized_w) {
      wr << "文章编号：" << doc.first << ":\n";
      for (const auto &mapp : doc.second) {
        wr << mapp.first << " " << fixed << setprecision(6) << mapp.second
           << "\n";
      }
    }
  }

private:
  vector<RSSIteam> _rss;
  // 某文章中，所有单词出现的次数,加入所有文章到容器
};

void processDirectory(const string &dir, RSS &rss, const string &storePath,
                      const string &offsetLib, const string &pageContents) {
  ArticleManager man(
      reader.Get("user", "dictPath", ""), reader.Get("user", "modelPath", ""),

      reader.Get("user", "userDictPath", ""), reader.Get("user", "idfPath", ""),
      reader.Get("user", "stopWordsPath", ""), 3, 5);

  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".xml") {
      std::cout << "Reading file: " << entry.path() << std::endl;
      rss.read(entry.path().string());
      rss.store(storePath, offsetLib, man, pageContents);
      bzero(&rss, sizeof(rss));
    }
  }
  // rss.buildInvertIndex();
  // rss.storeInvertIndextoRedis();
  // rss.storeIDF();
}

void test0() {
  RSS rss(50);
  processDirectory(reader.Get("user", "xmls", "UNKNOWN"), rss,
                   reader.Get("user", "xmlsStore", "UNKNOWN"),
                   reader.Get("user", "offsetLib", "UNKNOWN"),
                   reader.Get("user", "webPageContents", "UNKNOWN"));
}

int main(void) {
  test0();
  // cout << dict._cnStopsDir << endl;
  // cout << dict._enStopsDir << endl;
  return 0;
}

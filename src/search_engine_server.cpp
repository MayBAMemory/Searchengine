#include "../include/INIReader.h"
#include "../include/LRU.h"
#include "../include/inverted_index.h"
#include "../include/recommandWords.h"
#include "../include/returnTitle.h"
#include "../include/words_cut.h"
#include <wfrest/HttpServer.h>
#include <workflow/WFFacilities.h>
using namespace wfrest;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using namespace nlohmann;
using namespace protocol;

INIReader reader("conf/myconf.conf");
Redis redis(reader.Get("user", "redisServer", "UNKNOWN"));
// Redis redisSearch(reader.Get("user", "redisInvertIndex", "UNKNOWN"));
Redis redisCache("redis://127.0.0.1:6379/5");
cppjieba::Jieba jieba(reader.Get("user", "dictPath", "UNKNOWN"),
                      reader.Get("user", "modelPath", "UNKNOWN"),
                      reader.Get("user", "user_dict_path", "UNKNOWN"),
                      reader.Get("user", "idfPath", "UNKNOWN"),
                      reader.Get("user", "stopWords", "UNKNOWN"));
LRU_Cache articleCache(20, redisCache);

class CloudDiskServer {
public:
  CloudDiskServer(int cnt) : _waitGroup(cnt) {
    _invertedIndex = _invertIndex.loadInvertIndex("data/invertIndex.dat");
    _hashInvertedIndex = _invertIndex.getHashInvertIndex();
  }
  ~CloudDiskServer() {
    articleCache.clear();
  }
  void start(unsigned short port);
  void loadModules();

private:
  void loadSearchEngine();
  void loadRecommandEngine();
  void loadInvertIndex();

private:
  WFFacilities::WaitGroup _waitGroup;
  wfrest::HttpServer _httpserver;
  InvertedIndex _invertIndex;
  unordered_map<string, unordered_map<size_t, double>> _hashInvertedIndex;
  unordered_map<string, vector<pair<size_t, double>>> _invertedIndex;
};

void CloudDiskServer::start(unsigned short port) {
  if (_httpserver.track().start(port) == 0) {
    // _httpserver.Static("/static", "/home/shotlink/project_v1/tmp/");
    _httpserver.list_routes();
    _waitGroup.wait();
    _httpserver.stop();
  } else {
    printf("Cloudisk Server Start Failed!\n");
  }
}

void CloudDiskServer::loadModules() {
  loadInvertIndex();
  loadRecommandEngine();
  loadSearchEngine();
}

void CloudDiskServer::loadInvertIndex() {

  // vector<string> terms; // 存储所有term的key
  //
  // // 获取所有term
  // redisSearch.keys("*", back_inserter(terms));
  //
  // for (const auto &term : terms) {
  //   // 获取term对应的文档ID和权重
  //   vector<pair<string, double>> doc_weights;
  //   redisSearch.zrangebyscore(term, UnboundedInterval<double>{},
  //                             back_inserter(doc_weights)); // 获取Sorted
  //                             Set内容
  //
  //   for (const auto &[doc_id_str, weight] : doc_weights) {
  //     size_t doc_id = stoul(doc_id_str);   // 将doc_id从字符串转换为整数
  //     _invertIndex[doc_id][term] = weight; // 构建哈希表
  //   }
  // }
}

// 对URI进行解码
string decodeURIComponent(const string &encoded) {
  string decoded;
  decoded.reserve(encoded.size());

  for (size_t i = 0; i < encoded.length(); ++i) {
    if (encoded[i] == '%') {
      if (i + 2 < encoded.length()) {
        int value;
        std::istringstream hex_stream(encoded.substr(i + 1, 2));
        hex_stream >> std::hex >> value;
        decoded += static_cast<char>(value);
        i += 2;
      }
    } else {
      decoded += encoded[i];
    }
  }
  return decoded;
}

void CloudDiskServer::loadRecommandEngine() {
  _httpserver.GET("/s", [](const HttpReq *req, HttpResp *resp) {
    resp->set_header_pair("Access-Control-Allow-Origin", "*");
    resp->set_header_pair("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    resp->set_header_pair("Access-Control-Allow-Headers", "Content-Type");
    // if (req->content_type() == APPLICATION_URLENCODED) {
    //   auto form_kv = req->form_kv();
    //   string wword = decodeURIComponent(form_kv["wd"]);
    string word = req->query("wd");
    vector<string> words;
    string wword = decodeURIComponent(word);
    // wstring_convert<codecvt_utf8<wchar_t>> converter;
    // wstring wword = converter.from_bytes(word);

    recommandWords(redis, wword, words);
    for (auto const &ci : words) {
      resp->String(ci);
      resp->String(" ");
      // }
    }
  });
}

void CloudDiskServer::loadSearchEngine() {
  _httpserver.POST("/s", [this](const HttpReq *req, HttpResp *resp) {
    resp->set_header_pair("Access-Control-Allow-Origin", "*");
    resp->set_header_pair("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    resp->set_header_pair("Access-Control-Allow-Headers", "Content-Type");
    string word = req->query("wd");
    string wword = decodeURIComponent(word);
    vector<string> words;
    jieba.Cut(wword, words, true);

    // vector<double> base = buildBase(words);
    // for(auto const& zhi:base){
    //   cout<<"向量值："<<zhi<<"\n";
    // }
    //
    vector<pair<size_t, double>> docIds;
    vector<string> Titles;
    unordered_map<int, offsetInfo> offsets;

    processQuery(words, _invertedIndex, _hashInvertedIndex, docIds);
    cout << "获取到的文章数量：" << docIds.size() << "\n";
    // docIds = wwwords;
    // searchWords(redisSearch, words, _invertIndex.getInvertIndex(), docIds);
    vector<Article> articles;
    for (auto const &[docId, weight] : docIds) {
      // cout <<"docId: " <<docId << ", weight: "<<weight<<"\n";
      auto cacheArticle = articleCache.get(docId);
      if (cacheArticle) {
        cout<<"缓存命中"<<"\n";
        // 缓存命中，使用缓存文章
        articles.emplace_back(*cacheArticle);
      } else {
        // 缓存未命中，老套路从文件读
        offsets = loadOffsets();
        if (offsets.find(docId) != offsets.end()) {
          pair<string, string> title_content =
              getTitleContentById(offsets[docId]);
          //尝试存入缓存
          Article article(title_content);
          articleCache.put(docId, article);
          articles.emplace_back(title_content);
        }
      }
    }

    json respJson = articles;
    // respJson["titles"] = Titles;
    string jsonStr = respJson.dump();
    resp->String(jsonStr);

    // for (string const &title : Titles) {
    // resp->Strig(title);
    //   resp->String(" ");
    // }
  });
}

int main() {
  // DictProducer dict;
  // dict.buildEnDict(dict._dictDir);
  // dict.load_dict_and_store_to_redis(dict._dictDir, redis);

  // dict.buildCnDict(dict._cnYuliaoDir, dict._dictDire, jieba);
  // dict.save_dict_to_redis(redis);
  cout << "服务器开启中。。。" << endl;
  CloudDiskServer server(1);
  server.loadModules();
  server.start(8899);
  return 0;
}

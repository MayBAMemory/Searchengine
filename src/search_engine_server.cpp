#include "../include/INIReader.h"
#include "../include/inverted_index.h"
#include "../include/recommandWords.h"
#include "../include/words_cut.h"
#include <codecvt>
#include <regex>
#include <wfrest/HttpServer.h>
#include <workflow/WFFacilities.h>
using namespace wfrest;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using namespace protocol;

INIReader reader("conf/myconf.conf");
Redis redis(reader.Get("user", "redisServer", "UNKNOWN"));
Redis redisSearch(reader.Get("user", "redisInvertIndex", "UNKNOWN"));
cppjieba::Jieba jieba(reader.Get("user", "dictPath", "UNKNOWN"),
                      reader.Get("user", "modelPath", "UNKNOWN"),
                      reader.Get("user", "user_dict_path", "UNKNOWN"),
                      reader.Get("user", "idfPath", "UNKNOWN"),
                      reader.Get("user", "stopWords", "UNKNOWN"));

class CloudDiskServer {
public:
  CloudDiskServer(int cnt) : _waitGroup(cnt) {}
  ~CloudDiskServer() {}
  void start(unsigned short port);
  void loadModules();

private:
  void loadSearchEngine();
  void loadRecommandEngine();

private:
  WFFacilities::WaitGroup _waitGroup;
  wfrest::HttpServer _httpserver;
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
  loadRecommandEngine(); 
  loadSearchEngine();
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
  _httpserver.POST("/s", [](const HttpReq *req, HttpResp *resp) {
    resp->set_header_pair("Access-Control-Allow-Origin", "*");
    resp->set_header_pair("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    resp->set_header_pair("Access-Control-Allow-Headers", "Content-Type");
    string word = req->query("wd");
    string wword = decodeURIComponent(word);
    vector<string> words;
    jieba.Cut(wword, words,true);
    unordered_map<string, size_t> userTF;
    vector<size_t> docIds;
    searchWords(redisSearch, words, docIds);
    for (double const &docId : docIds) {
      cout << docId << "\n";
    }
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

#include "include/INIReader.h"
#include "include/recommandWords.h"

int main() {
  INIReader reader("conf/myconf.conf");
  if (reader.ParseError() < 0) {
    std::cout << "Can't load 'test.ini'\n";
    return 1;
  }
  Redis redis(reader.Get("user", "redisServer", "UNKNOWN"));
  string word;
  cout<<"输入：";
  cin >> word;
  recommandWords(redis, word);
  return 0;
}

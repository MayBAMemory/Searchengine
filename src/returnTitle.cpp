#include "../include/returnTitle.h"

unordered_map<int, offsetInfo> loadOffsets() {
  unordered_map<int, offsetInfo> offsets;
  ifstream offsetFile("data/offsetLib.dat");

  size_t id;
  long start, length;
  while (offsetFile >> id >> start >> length) {
    offsets[id] = {start, length};
  }
  offsetFile.close();
  return offsets;
}

string getTitleById(const offsetInfo &offset) {
  ifstream pageFile("data/ripepage.dat");
  pageFile.seekg(offset.start);
  string data(offset.length, '\0');
  pageFile.read(&data[0], offset.length);
  pageFile.close();

  size_t titleStart = data.find("<title>");
  size_t titleEnd = data.find("</title>");
  //若没有没有找到
  if (titleStart != string::npos && titleEnd != string::npos) {
    titleStart += string("<title>").length();
    return data.substr(titleStart, titleEnd - titleStart);
  }

  return data;
}

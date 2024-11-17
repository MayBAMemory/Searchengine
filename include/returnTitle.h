#include <fstream>
#include <iostream>
#include <unordered_map>

using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::unordered_map;

struct offsetInfo {
  long start;
  long length;
};

unordered_map<int, offsetInfo> loadOffsets(); 

string getTitleById(const offsetInfo &offset);

// int main() {
//   auto off = loadOffsets();
//   int id = 0;
//   std::cout << "输入文章id：";
//   std::cin >> id;
//
//   if (off.find(id) != off.end()) {
//     string title = getTitleById(off[id]);
//     std::cout << title << endl;
//   }
// }

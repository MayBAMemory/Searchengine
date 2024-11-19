#include <iostream>
#include <string>
#include <sw/redis++/redis++.h>
#include <unordered_map>
#include <vector>
using namespace sw::redis;
using std::cerr;
using std::pair;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

class InvertedIndex {
private:
  // unordered_map<size_t, unordered_map<string, double>> _w;
  unordered_map<string, unordered_map<size_t, double>> _hashInvertIndex;

public:
  // InvertedIndex();
  // _redisUrl(redisUrl) {}

  void storeInvertIndex(unordered_map<size_t, unordered_map<string, double>> w);
  unordered_map<string, unordered_map<size_t, double>> getHashInvertIndex();
  unordered_map<string, vector<pair<size_t, double>>>
  loadInvertIndex(const string &filename);
  void loadHashInvertIndex(const string &filename);

  // string _redisUrl;
  // void storeDocument();
  // void queryTerm(const string &term, vector<size_t> &docIds);
};
// void saveIndex(const unordered_map<size_t, unordered_map<string, double>>
// &index,
//                const string &filename);
//
// unordered_map<size_t, unordered_map<string, double>> loadIndex(const string
// &filename);
void
processQuery(const vector<string> &terms,
             unordered_map<string, vector<pair<size_t, double>>> index,
             unordered_map<string, unordered_map<size_t, double>> normalized_w,
             vector<pair<size_t, double>>& docIds,
             size_t top_k = 10);

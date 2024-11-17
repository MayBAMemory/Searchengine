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
  unordered_map<size_t, unordered_map<string, double>> _w;
public:
  InvertedIndex(unordered_map<size_t, unordered_map<string, double>> w,const string redisUrl):_w(w),_redisUrl(redisUrl){
  }

  string _redisUrl;
  void
  storeDocument();

  void queryTerm(const string &term, vector<size_t>& docIds);


  
};

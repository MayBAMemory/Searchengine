#include "../include/shortest.h"
#include <iostream>
#include <sw/redis++/command_options.h>
#include <sw/redis++/redis++.h>
#include <vector>
using namespace std;
using namespace sw::redis;

struct WordInfo {
  string word;
  double freq;
  int distance;

  WordInfo(string w, double f, int d) : word(w), freq(f), distance(d) {}
};

void recommandWords(Redis &redis, const string word, vector<string> &recWords);

void searchWords(Redis &redis, const vector<string> &terms,
                 unordered_map<size_t, unordered_map<string, double>> &normalized_w,
                 vector<pair<size_t,double>> &DocId);

vector<double> buildBase(const vector<string>& words);

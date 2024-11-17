#include "../include/article_manager.h"
using std::string;

ArticleManager::ArticleManager(const string &dictPath, const string &modelPath,
                               const string &userDictPath,
                               const string &idfPath,
                               const string &stopWordsPath, size_t threshold,
                               size_t topKeywords)
    : simhasher(dictPath, modelPath, userDictPath, idfPath, stopWordsPath),
      hammingThreshold(threshold), topN(topKeywords) {}

// 检查文章是否重复，返回重复文章的ID，如果不重复返回-1
int ArticleManager::checkDuplicate(const string &content) {
  uint64_t newHash = 0;
  simhasher.make(content, topN, newHash);

  for (const auto &pair : articleSimhashes) {
    if (Simhasher::isEqual(newHash, pair.second, hammingThreshold)) {
      return pair.first; // 返回重复文章的ID
    }
  }
  return -1; // 没有重复
}

// 添加新文章
bool ArticleManager::addArticle(int id, const string &content) {
  // if (id == 1) {
  //   uint64_t hash = 0;
  //   simhasher.make(content, topN, hash);
  //   articleSimhashes[id] = hash;
  // }
  int ret = checkDuplicate(content);
  // cout<<ret<<endl;
  
  // 首先检查是否重复
  if (ret!= -1) {
    return false; // 文章重复，添加失败
  }

  // 计算simhash值
  uint64_t hash = 0;
  simhasher.make(content, topN, hash);

  // 存储文章的simhash值
  articleSimhashes[id] = hash;
  return true;
}

// 删除文章
void ArticleManager::removeArticle(int id) { articleSimhashes.erase(id); }

// 获取当前文章数量
size_t ArticleManager::getArticleCount() const {
  return articleSimhashes.size();
}

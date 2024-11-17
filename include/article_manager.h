#include <string>
#include <unordered_map>
#include "../include/simhash/Simhasher.hpp"
#include <iostream>
using namespace std;
using namespace simhash;

class ArticleManager {
private:
    // 存储文章ID和对应的simhash值
    std::unordered_map<int, uint64_t> articleSimhashes;
    // simhash计算器
    Simhasher simhasher;
    // 海明距离阈值
    size_t hammingThreshold;
    // 提取关键词数量
    size_t topN;

public:
    ArticleManager(
        const string& dictPath,
        const string& modelPath,
        const string& userDictPath,
        const string& idfPath,
        const string& stopWordsPath,
        size_t threshold,
        size_t topKeywords
    );

    // 检查文章是否重复，返回重复文章的ID，如果不重复返回-1
    int checkDuplicate(const string& content);

    // 添加新文章
    bool addArticle(int id, const string& content);

    // 删除文章
    void removeArticle(int id);

    // 获取当前文章数量
    size_t getArticleCount() const;
};



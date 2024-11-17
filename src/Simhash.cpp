#include <iostream>

//this define can avoid some logs which you don't need to care about.
#define LOGGER_LEVEL LL_WARN 

#include "../include/simhash/Simhasher.hpp"
using namespace simhash;
const string& dictPath = "dict/jieba.dict.utf8";
const string& modelPath = "dict/hmm_model.utf8";
const string& idfPath = "dict/idf.utf8";
const string& user_dict_path = "dict/user.dict.utf8";
const string& stopWords= "dict/stop_words.utf8";
int main(int argc, char** argv)
{
    Simhasher simhasher(dictPath,modelPath,user_dict_path,idfPath,stopWords);
    string s("习近平，男，汉族，1953年6月生，陕西富平人，1969年1月参加工作，1974年1月加入中国共产党，清华大学人文社会学院马克思主义理论与思想政治教育专业毕业，在职研究生学历，法学博士学位。现任中国共产党中央委员会总书记，中共中央军事委员会主席，中华人民共和国主席，中华人民共和国中央军事委员会主席。");
    size_t topN = 5;
    uint64_t u64 = 0;
    vector<pair<string ,double> > res;
    simhasher.extract(s, res, topN);
    simhasher.make(s, topN, u64);
    cout<< "文本：\"" << s << "\"" << endl;
    cout << "关键词序列是: " << res << endl;
    cout<< "simhash值是: " << u64<<endl;


    const char * bin1 = "100010110110";
    const char * bin2 = "110001110011";
    uint64_t u1, u2;
    u1 = Simhasher::binaryStringToUint64(bin1); 
    u2 = Simhasher::binaryStringToUint64(bin2); 
    
    cout<< bin1 << "和" << bin2 << " simhash值的相等判断如下："<<endl;
    cout<< "海明距离阈值默认设置为3，则isEqual结果为：" << (Simhasher::isEqual(u1, u2)) << endl; 
    cout<< "海明距离阈值默认设置为5，则isEqual结果为：" << (Simhasher::isEqual(u1, u2, 5)) << endl; 
    return EXIT_SUCCESS;
}

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <string>
#include <errno.h>
#include <mysql/mysql.h>
#include "buffer.cpp"
#include "log.cpp"
#include "sqlConnectionPool.cpp"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    HttpRequest() {
        init();
    }
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer& buffer);
    bool isAlive();

    std::string path()      const;
    std::string& path();
    std::string method()    const;
    std::string version()   const;
    std::string getPost(const std::string& key)   const;
    std::string getPost(const char* key)   const;

private:
    bool parseRequestLine_(const std::string& line);
    void parseHeader_(const std::string& line);
    void parseBody_(const std::string& line);
    void parsePath_();
    void parsePost_();
    void parseFromUrlencoded_();

    static bool userVerify(const std::string& name,const std::string& password,bool isLogin);
    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string,std::string> header_;
    std::unordered_map<std::string,std::string> post_;
    
    static std::unordered_set<std::string> DEFAULT_HTML;
    static std::unordered_map<std::string,int> DEFAULT_HTML_TAG;
    static int converHex(char ch);
};
#endif
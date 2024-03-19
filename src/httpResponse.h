#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "buffer.cpp"
#include "log.cpp"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const std::string& srcDir,std::string& path,bool isAlice = false,int code = -1);
    void makeResponse(Buffer& buffer);
    void unmapFile();
    char* file();
    size_t fileLen() const;
    void errorContent(Buffer& buffer,std::string message);
    int Code() const;

private:
    void addStateLine_(Buffer& buffer);
    void addHeader_(Buffer& buffer);
    void addContent_(Buffer& buffer);
    void errorHtml_();
    std::string getFileType_();
    int code_;
    bool isAlive_;
    std::string path_, srcDir_;
    char* mmfile_;
    struct stat mmfileState_;
    static std::unordered_map<std::string,std::string> SUFFIX_TYPE;
    static std::unordered_map<int,std::string> CODE_STATUS;
    static std::unordered_map<int,std::string> CODE_PATH;
};
#endif
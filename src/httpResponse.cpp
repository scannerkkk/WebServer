#include "httpResponse.h"

using namespace std;
unordered_map<string,string> HttpResponse::SUFFIX_TYPE {
     { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "}
};


unordered_map<int,string> HttpResponse::CODE_STATUS {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" }
};

unordered_map<int,string> HttpResponse::CODE_PATH {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"}
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = srcDir_ = "";
    isAlive_ = false;
    mmfile_ = nullptr;
    mmfileState_ = {0};
}

HttpResponse::~HttpResponse() {
    unmapFile();
}

int HttpResponse::Code() const {
    return code_;
}

void HttpResponse::init(const string& srcDir,string& path,bool isAlive,int code) {
    assert(srcDir != "");
    if (mmfile_ != nullptr) {
        unmapFile();
    }
    code_ = code;
    isAlive_ = isAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmfile_ = nullptr;
    mmfileState_ = {0};
}

void HttpResponse::makeResponse(Buffer& buffer) {
    if (stat((srcDir_ + path_).data(),&mmfileState_) < 0 || S_ISDIR(mmfileState_.st_mode)) {
        code_ = 404;
    } else if (!(mmfileState_.st_mode & S_IROTH)) {
        code_ = 403;
    } else if (code_ == -1) {
        code_ = 200;
    }
    errorHtml_();
    addStateLine_(buffer);
    addHeader_(buffer);
    addContent_(buffer);
}

char* HttpResponse::file() {
    return mmfile_;
}

size_t HttpResponse::fileLen() const {
    return mmfileState_.st_size;
}

void HttpResponse::errorHtml_() {
    if (CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH[code_];
        stat((srcDir_ + path_).data(),&mmfileState_);
    }
}

void HttpResponse::addStateLine_(Buffer& buffer) {
    string status;
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS[code_];
    } else {
        code_ = 400;
        status = CODE_STATUS[400];
    }
    buffer.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::addHeader_(Buffer& buffer) {
    buffer.Append("Connection: ");
    if (isAlive_) {
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alice: max=6, timeout=120\r\n");
    } else {
        buffer.Append("close\r\n");
    }
    buffer.Append("Content-type: " + getFileType_() + "\r\n");
}

void HttpResponse::addContent_(Buffer& buffer) {
    int srcFd = open((srcDir_ + path_).data(),O_RDONLY);
    if (srcFd < 0) {
        errorContent(buffer,"File NotFound!");
        return;
    }
    LOG_DEBUG("file path %s",(srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0,mmfileState_.st_size,PROT_READ,MAP_PRIVATE,srcFd,0);
    if (*mmRet == -1) {
        errorContent(buffer,"File NotFound!");
        return;
    }
    mmfile_ = (char*) mmRet;
    close(srcFd);
    buffer.Append("Content-length: " + to_string(mmfileState_.st_size) + "\r\n\r\n");
}

void HttpResponse::unmapFile() {
    if (mmfile_) {
        munmap(mmfile_,mmfileState_.st_size);
        mmfile_ = nullptr;
    }
}

string HttpResponse::getFileType_() {
    string::size_type idx = path_.find_last_of('.');
    if (idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE[suffix];
    }
    return "text/plain";
}

void HttpResponse::errorContent(Buffer& buffer,string message) {
    string body,status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_)) {
        status = CODE_STATUS[code_];
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer</em></body></html>";
    buffer.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buffer.Append(body);
}
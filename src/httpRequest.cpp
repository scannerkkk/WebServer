#include "httpRequest.h"
using namespace std;

unordered_set<string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login", "/welcome", "/video", "/picture"
};

unordered_map<string,int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html",0}, {"/login.html",1}
};

void HttpRequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

string HttpRequest::path() const {
    return path_;
}

string& HttpRequest::path() {
    return path_;
}

string HttpRequest::version() const {
    return version_;
}

string HttpRequest::method() const {
    return method_;
}

// 查看某个连接是否存活
bool HttpRequest::isAlive()  {
    if (header_.count("Connection")) {
        return version_ == "1.1" && header_["Connection"] == "keep-alice";
    }
    return false;
}

int HttpRequest::converHex(char ch) {
    if (isupper(ch)) {
        return ch - 'A' + 10;
    }
    else if (islower(ch)) {
        return ch - 'A' + 10;
    }
    return ch;
}

bool HttpRequest::parse(Buffer& buffer) {
    const char CRLF[] = "\r\n"; // 行结束符标志 回车换行
    if (buffer.ReadableBytes() <= 0) { // 没有东西可以读
        return false;
    }
    while (buffer.ReadableBytes() && state_ != FINISH) {
        auto end = search(buffer.Peek(),buffer.BeginWriteConst(),CRLF,CRLF + 2); // 找到去除\r\n的数据
        string line(buffer.Peek(),end);
        switch (state_) {
        case REQUEST_LINE:
            if (!parseRequestLine_(line)) {
                return false;
            } 
            parsePath_(); // 解析路径
            break;
        case HEADERS:
            parseHeader_(line);
            if (buffer.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            parseBody_(line);
            break;
        default:
            break;
        }

        if (end == buffer.BeginWrite()) { // 数据读完了
            break;
        }
        buffer.RetrieveUntil(end + 2); // 跳过回车换行
    }
    LOG_DEBUG("[%s],[%s],[%s]",method_.c_str(),path_.c_str(),version_.c_str());
    return true;
}

// 解析路径
void HttpRequest::parsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        if (DEFAULT_HTML.count(path_)) {
            path_ += ".html";
        }
    }
}

// 解析请求行
bool HttpRequest::parseRequestLine_(const string& line) {
    regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    if (regex_match(line,subMatch,pattern)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("parseRequestLine Error!");
    return false;
}

// 解析请求头
void HttpRequest::parseHeader_(const string& line) {
    regex pattern("^([^:]*): ?(.*)$");
    smatch subMatch;
    if (regex_match(line,subMatch,pattern)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = BODY;
    }
}

// 解析请求体
void HttpRequest::parseBody_(const string& line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body: %s, len:%d",line.c_str(),line.size());
}

// 处理post请求
void HttpRequest::parsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-from-urlencoded") {
        parseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) { // 如果是登陆/注册
            int tag = DEFAULT_HTML_TAG[path_];
            LOG_DEBUG("Tag:%d",tag);
            if (tag == 0 || tag == 1) {
                if (userVerify(post_["username"],post_["password"],tag == 1)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
        }
    }
}

// 解析url中编码
void HttpRequest::parseFromUrlencoded_() {
    if (body_.size() == 0) {
        return;
    }
    string key,value;
    int num = 0,n = body_.size(),i = 0,j = 0;
    while (i < n) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j,i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%': // 空格换为'+'或者%20
            num = (converHex(body_[i + 1]) << 4) + converHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j,i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s",key.c_str(),value.c_str());
            break;
        default:
            break;
        }
        i ++;
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j,i - j);
        post_[key] = value;
    }
}

bool HttpRequest::userVerify(const string& name,const string& password,bool isLoginTag) {
    if (name == "" || password == "") {
        return false;
    }
    LOG_INFO("Verify name:%s, password:%s",name.c_str(),password.c_str());
    MYSQL* sql = nullptr;
    SqlConnectionRall(&sql,SqlConnectionPool::instance());
    assert(sql != nullptr);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES* res = nullptr;

    if (!isLoginTag) {
        flag = true;
    }

    // 查询用户名和密码
    snprintf(order,256,"SELECT username,password FROM user WHERE username='%s' LIMIT 1",name.c_str());
    LOG_DEBUG("%s",order);
    if (mysql_query(sql,order)) {
        return false;
    }
    res = mysql_store_result(sql); // 将查询的结果放到result里面
    j = mysql_num_fields(res);      // 获取结果集列数
    fields = mysql_fetch_fields(res); // 获取一个所有字段结构的数组

    while (MYSQL_ROW row = mysql_fetch_row(res)) { // 登陆
        LOG_DEBUG("MYSQL ROW: %s %s",row[0],row[1]);
        string pwd(row[1]);
        if (isLoginTag) {
            if (pwd == password) {
                flag = true;
            } else {
                flag = false;
                LOG_INFO("password Error!");
            }
        } else { 
            flag = false;
            LOG_INFO("user User!");
        }
    }
    mysql_free_result(res);

    if (!isLoginTag && flag == true) { // 注册
        LOG_DEBUG("Register!");
        bzero(order,256);
        snprintf(order,256,"INSERT INTO user(username,password) VALUES('%s','%s')",name.c_str(),password.c_str());
        LOG_DEBUG("%s",order);
        if (mysql_query(sql,order)) {
            LOG_DEBUG("Register Error!");
            flag = false;
        }
    }
    LOG_DEBUG("UserVerify!");
    return flag;
}
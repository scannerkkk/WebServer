#include <features.h>
#include "webServer.cpp"

int main() {
    WebServer webServer(12345,3,60000,false,
                        3306,"root","123456","webServer",
                        12,6,true,0,1024);
    webServer.start();
}

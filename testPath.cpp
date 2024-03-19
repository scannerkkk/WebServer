#include "iostream"

#include <unistd.h>
int main()
{
    char* buf = getcwd(nullptr,0);
    std::cout << buf;
    return 0;
}
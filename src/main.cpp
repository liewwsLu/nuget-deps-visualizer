#include <iostream>
#include <string>
#include <vector>

#include "app.h"
#include "http.h"

#ifndef DEPVIZ_TESTS
/// Точка входа: передать параметры командной строки в приложение.
int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    CurlFetcher fetcher;
    return runApp(args, fetcher, std::cout, std::cerr);
}
#endif

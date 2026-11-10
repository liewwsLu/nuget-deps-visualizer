// Запуск всех unit-тестов: выводит результат каждого теста и итог.

#include <exception>
#include <iostream>

#include "test.h"

int main() {
    int failed = 0;
    for (const test::Case& testCase : test::registry()) {
        try {
            testCase.body();
            std::cout << "[  OK  ] " << testCase.name << "\n";
        } catch (const std::exception& error) {
            ++failed;
            std::cout << "[ FAIL ] " << testCase.name << "\n         " << error.what() << "\n";
        }
    }
    std::size_t total = test::registry().size();
    std::cout << "\nRan " << total << " tests: " << total - failed << " passed, " << failed << " failed\n";
    std::cout << (failed == 0 ? "OK" : "FAILED") << "\n";
    return failed == 0 ? 0 : 1;
}

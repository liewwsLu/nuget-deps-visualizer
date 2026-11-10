#pragma once

// Минимальный фреймворк для unit-тестов (без сторонних библиотек).
//
// TEST(имя) { ... }            — объявить тест;
// CHECK(условие)               — проверить, что условие истинно;
// CHECK_EQ(a, b)               — проверить равенство (значения выводятся при ошибке);
// CHECK_THROWS(выражение, Тип) — проверить, что выражение бросает исключение Тип;
// CHECK_CONTAINS(текст, часть) — проверить, что строка содержит подстроку.

#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test {

/// Проваленная проверка: где и что не выполнилось.
struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Зарегистрированный тест.
struct Case {
    std::string name;
    std::function<void()> body;
};

/// Список всех тестов программы.
inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

/// Регистрирует тест при запуске программы.
struct Registrar {
    Registrar(const std::string& name, std::function<void()> body) {
        registry().push_back({name, std::move(body)});
    }
};

/// Сообщение о проваленной проверке.
inline std::string where(const char* file, int line, const std::string& what) {
    return std::string(file) + ":" + std::to_string(line) + ": " + what;
}

/// Значение в виде строки для сообщения об ошибке.
template <typename T>
std::string show(const T& value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

inline std::string show(const std::vector<std::string>& values) {
    std::string result = "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        result += (i ? ", " : "") + values[i];
    }
    return result + "]";
}

inline std::string show(bool value) {
    return value ? "true" : "false";
}

}  // namespace test

#define TEST_CONCAT2(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT2(a, b)

#define TEST(name)                                                                     \
    static void name();                                                                \
    static test::Registrar TEST_CONCAT(registrar_, name)(#name, name);                 \
    static void name()

#define CHECK(condition)                                                               \
    do {                                                                               \
        if (!(condition)) {                                                            \
            throw test::Failure(test::where(__FILE__, __LINE__, "CHECK(" #condition ")")); \
        }                                                                              \
    } while (false)

#define CHECK_EQ(actual, expected)                                                     \
    do {                                                                               \
        auto actualValue = (actual);                                                   \
        auto expectedValue = (expected);                                               \
        if (!(actualValue == expectedValue)) {                                         \
            throw test::Failure(test::where(__FILE__, __LINE__,                        \
                                            #actual " == " #expected ": got " +        \
                                                test::show(actualValue) + ", expected " + \
                                                test::show(expectedValue)));           \
        }                                                                              \
    } while (false)

#define CHECK_THROWS(expression, ExceptionType)                                        \
    do {                                                                               \
        bool thrown = false;                                                           \
        try {                                                                          \
            (void)(expression);                                                        \
        } catch (const ExceptionType&) {                                               \
            thrown = true;                                                             \
        }                                                                              \
        if (!thrown) {                                                                 \
            throw test::Failure(test::where(__FILE__, __LINE__,                        \
                                            #expression " did not throw " #ExceptionType)); \
        }                                                                              \
    } while (false)

#define CHECK_CONTAINS(text, part)                                                     \
    do {                                                                               \
        std::string textValue = (text);                                                \
        std::string partValue = (part);                                                \
        if (textValue.find(partValue) == std::string::npos) {                          \
            throw test::Failure(test::where(__FILE__, __LINE__,                        \
                                            "\"" + partValue + "\" not found in \"" +  \
                                                textValue + "\""));                    \
        }                                                                              \
    } while (false)

#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/// Ошибка в тексте JSON.
struct JsonError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Значение JSON: null, логическое, число, строка, массив или объект.
struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    std::string text;                                     ///< строка или запись числа
    std::vector<JsonValue> items;                         ///< элементы массива
    std::vector<std::pair<std::string, JsonValue>> members;  ///< поля объекта по порядку

    /// Поле объекта по имени или nullptr, если поля нет.
    const JsonValue* find(const std::string& key) const;
};

/// Разобрать текст JSON (рекурсивный спуск по грамматике JSON).
JsonValue parseJson(const std::string& text);

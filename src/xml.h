#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/// Ошибка в тексте XML.
struct XmlError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Элемент XML: имя тега (без префикса пространства имён), атрибуты,
/// вложенные элементы и текст внутри.
struct XmlElement {
    std::string name;
    std::map<std::string, std::string> attributes;
    std::vector<XmlElement> children;
    std::string text;

    /// Значение атрибута или fallback, если атрибута нет.
    std::string attribute(const std::string& key, const std::string& fallback = "") const;

    /// Дочерние элементы с заданным именем.
    std::vector<const XmlElement*> childrenNamed(const std::string& childName) const;

    /// Первый элемент с заданным именем на любой глубине (обход в ширину) или nullptr.
    const XmlElement* findDeep(const std::string& searchName) const;
};

/// Разобрать текст XML и вернуть корневой элемент.
/// Пропускаются объявление <?xml ...?>, комментарии и <!DOCTYPE>.
XmlElement parseXml(const std::string& text);

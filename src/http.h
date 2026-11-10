#pragma once

#include <stdexcept>
#include <string>

/// Ошибка при обращении к репозиторию (нет связи, пакет не найден и т. п.).
struct RepositoryError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Ответ HTTP-сервера: код состояния и текст.
struct HttpResponse {
    int status = 0;
    std::string body;
};

/// Источник HTTP-ответов. В программе — CurlFetcher, в тестах — подмена
/// с заранее сохранёнными ответами сервера.
class Fetcher {
public:
    virtual ~Fetcher() = default;

    /// Выполнить GET-запрос. Если связи с сервером нет — RepositoryError.
    virtual HttpResponse get(const std::string& url) = 0;
};

/// Загрузка через системную утилиту curl (есть в Linux, macOS и Windows 10+).
/// Утилита только выполняет HTTP-запрос; разбор ответов делает программа.
class CurlFetcher : public Fetcher {
public:
    HttpResponse get(const std::string& url) override;
};

/// Разобрать вывод curl: текст ответа, затем на последней строке код HTTP.
HttpResponse parseCurlOutput(const std::string& output);

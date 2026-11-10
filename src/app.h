#pragma once

#include <ostream>
#include <string>
#include <vector>

#include "http.h"

/// Запустить приложение: args — параметры командной строки без имени программы,
/// fetcher — источник HTTP-ответов (в тестах — подмена), out — обычный вывод,
/// err — сообщения об ошибках. Возвращает код завершения.
int runApp(const std::vector<std::string>& args, Fetcher& fetcher, std::ostream& out, std::ostream& err);

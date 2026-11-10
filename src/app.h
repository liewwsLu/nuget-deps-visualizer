#pragma once

#include <ostream>
#include <string>
#include <vector>

/// Запустить приложение: args — параметры командной строки без имени программы,
/// out — обычный вывод, err — сообщения об ошибках. Возвращает код завершения.
int runApp(const std::vector<std::string>& args, std::ostream& out, std::ostream& err);

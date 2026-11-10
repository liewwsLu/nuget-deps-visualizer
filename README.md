# Визуализатор графа зависимостей NuGet

Практическая работа №2 по дисциплине «Конфигурационное управление», вариант 28.

## Общее описание

Консольное приложение (CLI) на C++17 для визуализации графа зависимостей
пакетов менеджера NuGet (.NET). Готовые менеджеры пакетов и сторонние
библиотеки для получения зависимостей не используются: только стандартная
библиотека C++.

**Этап 1. Минимальный прототип с конфигурацией.** Приложение принимает
параметры командной строки, проверяет их и выводит все параметры в формате
`ключ=значение`. Получение и анализ зависимостей будут добавлены на следующих
этапах.

## Функции и настройки

### Параметры командной строки

| Параметр | Обязательный | По умолчанию | Что задаёт |
|---|---|---|---|
| `--package ИМЯ` | да | — | имя анализируемого пакета |
| `--repo URL_ИЛИ_ПУТЬ` | нет | `https://api.nuget.org/v3/index.json` | URL репозитория NuGet или путь к файлу тестового репозитория |
| `--test-mode` | нет | выключен | режим работы с тестовым репозиторием |
| `--output ФАЙЛ` | нет | `graph.svg` | имя файла с изображением графа |
| `--ascii-tree` | нет | выключен | вывод зависимостей в виде ASCII-дерева |

Значение можно писать через пробел (`--package Serilog`) или через знак
равенства (`--package=Serilog`).

### Проверка параметров

Любая ошибка выводится на экран с префиксом `error:` и подсказкой `usage:`,
программа завершается с кодом 1.

| Параметр | Ошибка |
|---|---|
| `--package` | не задан; нет значения; пустой; недопустимые символы; длиннее 100 символов; в тестовом режиме — не большие латинские буквы |
| `--repo` | нет значения; пустой; не `http://` или `https://` адрес; недопустимые символы в URL; в тестовом режиме — не задан или файл не существует |
| `--output` | нет значения; расширение не `.svg`; папка для файла не существует |
| `--ascii-tree`, `--test-mode` | флаги, значение передавать нельзя |
| любой | неизвестный параметр; параметр задан дважды; аргумент без `--` |

### Вывод параметров (только для этапа 1)

```
package=Serilog
repository=https://api.nuget.org/v3/index.json
test_mode=false
output=graph.svg
ascii_tree=false
```

## Сборка и запуск тестов

Нужен компилятор C++17: `g++` (Linux; Windows — MinGW-w64). Сторонние
библиотеки не нужны.

Скрипты `run.sh` / `run.bat` собирают программу в папку `bin` и сразу
запускают её. Сборка выполняется при каждом запуске, поэтому после
переключения на другой коммит работает код именно этого коммита.

```
./run.sh --package Serilog          # Linux, macOS
run.bat --package Serilog           # Windows
```

Ручная сборка:

```
g++ -std=c++17 -O2 -o bin/depviz src/*.cpp
```

Unit-тесты собираются из исходников программы и файлов `tests/*.cpp` и
запускаются скриптами:

```
./test.sh                           # Linux, macOS
test.bat                            # Windows
```

Для тестов используется собственный минимальный фреймворк `tests/test.h`
(макросы `TEST`, `CHECK`, `CHECK_EQ`, `CHECK_THROWS`, `CHECK_CONTAINS`) —
без сторонних библиотек. Программа выводит результат каждого теста и итог:
`Ran N tests: N passed, 0 failed`.

| Файл тестов | Что проверяет |
|---|---|
| `tests/test_config.cpp` | значения по умолчанию, все параметры, запись через `=`, вывод ключ=значение, ошибки каждого параметра, код возврата |

## Примеры использования

```
$ ./run.sh --package Serilog --ascii-tree
package=Serilog
repository=https://api.nuget.org/v3/index.json
test_mode=false
output=graph.svg
ascii_tree=true

$ ./run.sh --package A --test-mode --repo tests/data/simple.txt --output a.svg
package=A
repository=tests/data/simple.txt
test_mode=true
output=a.svg
ascii_tree=false

$ ./run.sh --package Serilog --output graph.png
error: output file must have .svg extension: graph.png
usage: depviz --package NAME [--repo URL_OR_PATH] [--test-mode] [--output FILE.svg] [--ascii-tree]

$ ./run.sh --package A --test-mode
error: test mode requires --repo with a path to a test file

$ ./run.sh --package Serilog --color red
error: unknown option: --color
```

## Структура проекта

```
src/
  main.cpp      точка входа
  app.h/.cpp    запуск приложения, вывод результата
  config.h/.cpp параметры командной строки и их проверка
tests/
  test.h           минимальный фреймворк unit-тестов
  test_main.cpp    запуск всех тестов
  helpers.h        запуск приложения и временные файлы в тестах
  test_config.cpp  тесты параметров
  data/            файлы тестовых репозиториев
run.sh, run.bat    сборка и запуск приложения
test.sh, test.bat  сборка и запуск тестов
```

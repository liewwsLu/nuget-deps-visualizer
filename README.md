# Визуализатор графа зависимостей NuGet

Практическая работа №2 по дисциплине «Конфигурационное управление», вариант 28.

## Общее описание

Консольное приложение (CLI) на C++17 для визуализации графа зависимостей
пакетов менеджера NuGet (.NET). Готовые менеджеры пакетов и сторонние
библиотеки для получения зависимостей не используются: только стандартная
библиотека C++.

**Этап 2. Сбор данных.** Приложение подключается к репозиторию NuGet по
протоколу NuGet V3, находит последнюю стабильную версию заданного пакета,
скачивает его описание `.nuspec` и выводит прямые зависимости пакета.
Тестовый репозиторий и полный граф зависимостей будут добавлены на
следующих этапах.

### Как получаются зависимости

1. По URL репозитория (`https://api.nuget.org/v3/index.json`) скачивается
   индекс сервиса — JSON со списком ресурсов. Из него берётся ресурс
   `PackageBaseAddress/3.0.0` — адрес хранилища пакетов
   (`https://api.nuget.org/v3-flatcontainer/`).
2. По адресу `{хранилище}/{имя}/index.json` скачивается список версий
   пакета. Выбирается последняя стабильная (без метки `-beta`, `-dev`…).
   Версии сравниваются по числам: `1.10.0` новее `1.9.0`.
3. По адресу `{хранилище}/{имя}/{версия}/{имя}.nuspec` скачивается описание
   пакета — XML-файл. В разделе `<dependencies>` зависимости разбиты на группы
   по целевым платформам (`.NETStandard2.0`, `net8.0` и т. д.).
4. Выбирается одна группа: `netstandard2.0` (подходит большинству проектов),
   иначе `netstandard2.1`, иначе зависимости без группы, иначе первая группа.

Имена пакетов в адресах пишутся маленькими буквами — так требует протокол.

### Чем это сделано

В стандартной библиотеке C++ нет ни HTTP, ни JSON, ни XML, поэтому:

| Задача | Решение |
|---|---|
| HTTP-запрос | системная утилита `curl` (есть в Linux, macOS и Windows 10+), запускается через `popen`; она только скачивает текст, код ответа (200, 404…) программа читает сама |
| JSON (индекс сервиса, список версий) | собственный разборщик `src/json.cpp` — рекурсивный спуск по грамматике JSON |
| XML (файл `.nuspec`) | собственный разборщик `src/xml.cpp`: теги, атрибуты, комментарии, CDATA, ссылки `&amp;` и т. п. |

Сетевой доступ скрыт за интерфейсом `Fetcher`: в программе это `CurlFetcher`,
в тестах — `FakeFetcher` с заранее сохранёнными ответами сервера.

## Функции и настройки

### Параметры командной строки

| Параметр | Обязательный | По умолчанию | Что задаёт |
|---|---|---|---|
| `--package ИМЯ` | да | — | имя анализируемого пакета |
| `--repo URL_ИЛИ_ПУТЬ` | нет | `https://api.nuget.org/v3/index.json` | URL репозитория NuGet (индекс сервиса или сразу адрес хранилища) или путь к файлу тестового репозитория |
| `--test-mode` | нет | выключен | режим работы с тестовым репозиторием |
| `--output ФАЙЛ` | нет | `graph.svg` | имя файла с изображением графа |
| `--ascii-tree` | нет | выключен | вывод зависимостей в виде ASCII-дерева |

### Ошибки

Любая ошибка выводится с префиксом `error:`, программа завершается с кодом 1.

| Источник | Ошибки |
|---|---|
| `--package` | не задан; нет значения; пустой; недопустимые символы; длиннее 100 символов; в тестовом режиме — не большие латинские буквы |
| `--repo` | нет значения; пустой; не `http://` или `https://` адрес; недопустимые символы; в тестовом режиме — не задан или файл не существует |
| `--output` | нет значения; расширение не `.svg`; папка для файла не существует |
| остальное | неизвестный параметр; параметр задан дважды; флаг со значением |
| репозиторий | нет связи (или не установлен `curl`); пакет не найден; версия не найдена; сервер вернул код ошибки; в индексе нет `PackageBaseAddress/3.0.0`; повреждённый JSON или XML |

Параметр `--test-mode` принимается, но на этом этапе выдаёт ошибку
`test repository mode is not supported yet`: тестовый репозиторий
добавляется на этапе 3.

### Вывод прямых зависимостей (только для этапа 2)

```
Package: Serilog.Sinks.Console 6.1.1
Framework: .NETStandard2.0
Direct dependencies:
  Serilog >= 4.0.0
```

Версия `4.0.0` в `.nuspec` означает «4.0.0 или новее», поэтому выводится
как `>= 4.0.0`. Диапазон вида `[1.0, 2.0)` выводится как есть. Если
зависимостей нет — `(none)`.

## Сборка и запуск тестов

Нужны компилятор C++17 (`g++`; на Windows — MinGW-w64) и утилита `curl`
(для работы с сетью; тестам она не нужна).

```
./run.sh --package Serilog          # Linux, macOS: сборка в bin/ и запуск
run.bat --package Serilog           # Windows

./test.sh                           # unit-тесты, Linux, macOS
test.bat                            # unit-тесты, Windows
```

Тесты не обращаются к интернету: ответы сервера NuGet берутся из файлов
`tests/data/nuget/` (индекс сервиса, список версий и `.nuspec` пакета
Serilog.Sinks.Console, `.nuspec` старого формата и без зависимостей).

| Файл тестов | Что проверяет |
|---|---|
| `tests/test_config.cpp` | значения по умолчанию, все параметры, ошибки каждого параметра |
| `tests/test_json.cpp` | объекты, массивы, `true`/`false`/`null`, числа, экранирование `\"` и `\uXXXX`, ошибки в JSON |
| `tests/test_xml.cpp` | теги и атрибуты, префиксы пространств имён, `&amp;` и CDATA, поиск вложенного тега, ошибки в XML |
| `tests/test_versions.cpp` | сравнение версий по числам, пререлизы, метаданные `+build`, выбор последней стабильной |
| `tests/test_nuspec.cpp` | группы платформ, старый формат без групп, пустые группы, зависимость без `id`, выбор группы |
| `tests/test_nuget.cpp` | адрес хранилища из индекса, прямой адрес, коды 404 и 500, нет связи, BOM, имена маленькими буквами, разбор вывода `curl` |
| `tests/test_app.cpp` | запуск приложения целиком: вывод зависимостей, `(none)`, ошибки и коды возврата |

## Примеры использования

```
$ ./run.sh --package Serilog.Sinks.Console
Package: Serilog.Sinks.Console 6.1.1
Framework: .NETStandard2.0
Direct dependencies:
  Serilog >= 4.0.0

$ ./run.sh --package Microsoft.Extensions.Logging
Package: Microsoft.Extensions.Logging 10.0.14
Framework: .NETStandard2.0
Direct dependencies:
  Microsoft.Bcl.AsyncInterfaces >= 10.0.14
  Microsoft.Extensions.DependencyInjection >= 10.0.14
  Microsoft.Extensions.Logging.Abstractions >= 10.0.14
  Microsoft.Extensions.Options >= 10.0.14
  System.Diagnostics.DiagnosticSource >= 10.0.14

$ ./run.sh --package Newtonsoft.Json
Package: Newtonsoft.Json 13.0.4
Framework: .NETStandard2.0
Direct dependencies:
  (none)

$ ./run.sh --package No.Such.Package.Xyz
error: package not found: No.Such.Package.Xyz

$ ./run.sh --package Serilog --repo https://example.invalid/v3/index.json
error: cannot connect to https://example.invalid/v3/index.json (check the network and that curl is installed)

$ ./run.sh --package A --test-mode --repo tests/data/simple.txt
error: test repository mode is not supported yet
usage: depviz --package NAME [--repo URL_OR_PATH] [--test-mode] [--output FILE.svg] [--ascii-tree]
```

## Структура проекта

```
src/
  main.cpp          точка входа
  app.h/.cpp        запуск приложения, вывод результата
  config.h/.cpp     параметры командной строки и их проверка
  http.h/.cpp       интерфейс Fetcher и загрузка через curl
  json.h/.cpp       разбор JSON
  xml.h/.cpp        разбор XML
  versions.h/.cpp   сравнение версий
  nuspec.h/.cpp     зависимости из файла .nuspec
  nuget.h/.cpp      репозиторий NuGet: индекс, версии, .nuspec
tests/
  test.h            минимальный фреймворк unit-тестов
  test_main.cpp     запуск всех тестов
  helpers.h         запуск приложения и временные файлы в тестах
  fake_nuget.h      подмена сети в тестах
  test_*.cpp        unit-тесты
  data/nuget/       сохранённые ответы сервера NuGet
  data/*.txt        файлы тестовых репозиториев
run.sh, run.bat     сборка и запуск приложения
test.sh, test.bat   сборка и запуск тестов
```

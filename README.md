# Визуализатор графа зависимостей NuGet

Практическая работа №2 по дисциплине «Конфигурационное управление», вариант 28.

## Общее описание

Консольное приложение (CLI) на C++17 для визуализации графа зависимостей
пакетов менеджера NuGet (.NET). Готовые менеджеры пакетов и сторонние
библиотеки для получения зависимостей не используются: только стандартная
библиотека C++.

**Этап 4. Дополнительные операции.** Приложение строит полный граф
зависимостей пакета с учётом транзитивности, находит циклы, поддерживает
тестовый режим (граф пакетов из текстового файла) и режим вывода **порядка
загрузки** зависимостей (`--load-order`). Результаты сравниваются с
менеджером пакетов NuGet (`dotnet`).

### Как получаются зависимости пакета NuGet

1. По URL репозитория (`https://api.nuget.org/v3/index.json`) скачивается
   индекс сервиса (JSON). Из него берётся ресурс `PackageBaseAddress/3.0.0` —
   адрес хранилища пакетов (`https://api.nuget.org/v3-flatcontainer/`).
2. `{хранилище}/{имя}/index.json` — список версий пакета.
3. `{хранилище}/{имя}/{версия}/{имя}.nuspec` — описание пакета (XML), в нём
   зависимости, разбитые на группы по целевым платформам.
4. Берётся группа `netstandard2.0`, иначе `netstandard2.1`, иначе зависимости
   без группы, иначе первая группа.

HTTP-запросы выполняет системная утилита `curl`; JSON и XML разбираются
собственными разборщиками (`src/json.cpp`, `src/xml.cpp`).

### Выбор версий

* Анализируемый пакет — последняя стабильная версия.
* Зависимость — **наименьшая** версия, подходящая под диапазон из `.nuspec`
  (правило NuGet «lowest applicable version»):

  | Запись в `.nuspec` | Значение |
  |---|---|
  | `1.0` | 1.0 или новее |
  | `[1.0, 2.0)` | от 1.0 включительно до 2.0 не включая |
  | `(1.0,)` | новее 1.0 |
  | `[1.0]` | ровно 1.0 |
  | `(, 2.0]` | до 2.0 включительно |

  Пререлизы (`-beta`, `-dev`…) не берутся, если граница сама не пререлиз.
* Если пакет встречается несколько раз, используется диапазон, встреченный
  первым.
* Имена пакетов NuGet не зависят от регистра: `lib.c` и `Lib.C` — один пакет.

### Построение графа: DFS без рекурсии

Граф строится обходом в глубину (DFS) **без рекурсии** — с явным стеком
(`src/graph.cpp`, функция `buildGraph`). В стеке (`std::vector<Frame>`) лежат
пары «пакет, номер следующей зависимости». У каждого пакета есть состояние:
«в обработке» (лежит на стеке) или «готов». На каждом шаге берётся верхний
пакет и его следующая зависимость:

* все зависимости пакета просмотрены — пакет снимается со стека, состояние
  «готов»;
* зависимость ещё не встречалась — её зависимости запрашиваются, она кладётся
  на стек;
* зависимость в состоянии «готов» — пропускается (каждый пакет запрашивается
  один раз, даже если от него зависят несколько пакетов);
* зависимость в состоянии «в обработке» — это **цикл**: путь от неё по стеку
  до текущего пакета запоминается, дальше по этой связи обход не идёт, поэтому
  программа не зацикливается.

Без рекурсии глубина графа ограничена только памятью: цепочка из 200 000
пакетов (unit-тест `graph_no_stack_overflow_on_long_chain`) обрабатывается
без переполнения стека вызовов.

Источник данных скрыт за интерфейсом `DependencySource` с двумя методами —
`dependencies(имя)` и `label(имя)`. Его реализуют `NuGetSource` (репозиторий
NuGet) и `LocalRepository` (тестовый файл), поэтому обход один и тот же.

### Порядок загрузки

С параметром `--load-order` выводится порядок, в котором нужно загрузить
пакеты, чтобы к моменту загрузки каждого пакета все его зависимости уже были
загружены. Анализируемый пакет загружается последним.

Порядок строит функция `loadOrder` (`src/graph.cpp`) — тоже обходом в
глубину без рекурсии, с тем же явным стеком: пакет добавляется в список,
когда обработаны все его зависимости (порядок завершения обхода, post-order).
Это топологическая сортировка графа. Если в графе есть цикл, строгого порядка
не существует: связь, замыкающая цикл, пропускается, и программа сообщает
об этом строкой `Note:`.

```
$ ./run.sh --package A --test-mode --load-order --repo tests/data/cycle.txt
Load order of A (dependencies first):
  1. D
  2. C
  3. B
  4. A
Note: cycle A -> B -> C -> A - its edge C -> A was ignored
```

### Сравнение с менеджером пакетов NuGet

Проверка: создать консольный проект .NET, добавить пакет и вывести все его
зависимости штатной командой:

```
dotnet new console -o demo
cd demo
dotnet add package Serilog.Sinks.Console
dotnet list package --include-transitive
```

Результаты проверки (ноябрь 2026, консольный проект .NET 10, версии
на момент проверки):

| Пакет | `dotnet list package` | Наша программа |
|---|---|---|
| `Serilog.Sinks.Console 6.1.1` | 2 пакета: сам пакет и `Serilog 4.0.0` | 9 пакетов: те же 2 и ещё 7 пакетов `System.*` (`System.Memory`, `System.Buffers`, `System.Threading.Channels` и др.) |
| `Microsoft.Extensions.Logging 10.0.14` | 6 пакетов (`Logging`, `Logging.Abstractions`, `DependencyInjection`, `DependencyInjection.Abstractions`, `Options`, `Primitives`) | 14 пакетов: те же 6 и ещё `Microsoft.Bcl.AsyncInterfaces`, `System.ComponentModel.Annotations` и 6 пакетов `System.*` |
| `Microsoft.Extensions.Http 10.0.14` | 13 пакетов | 16 пакетов: 8 общих и 8 пакетов для старых платформ; нет 5 пакетов `Configuration`, `Configuration.Binder`, `Diagnostics`, `Diagnostics.Abstractions`, `Options.ConfigurationExtensions` |

Версии общих пакетов совпадают (`Serilog 4.0.0`, все
`Microsoft.Extensions.*` — `10.0.14`). Состав и порядок различаются, и вот
почему:

1. **Целевая платформа — главная причина.** В `.nuspec` зависимости
   записаны отдельно для каждой платформы. NuGet берёт группу под платформу
   проекта (`net10.0`), наша программа — группу `netstandard2.0`. Пакеты
   `System.Memory`, `System.Buffers`, `System.Threading.Channels`,
   `Microsoft.Bcl.AsyncInterfaces` и т. п. — это дополнения для старых
   платформ (.NET Framework, .NET Standard). В .NET 10 всё это встроено,
   поэтому в группе `net10.0` их нет, и NuGet их не ставит. И наоборот:
   у `Microsoft.Extensions.Http` для `net10.0` есть зависимости
   (`Diagnostics`, `Configuration.Binder` и др.), которых нет в группе
   `netstandard2.0`.
2. **Порядок вывода.** NuGet не выводит порядок загрузки: пакеты
   скачиваются параллельно, а `dotnet list package` и файл
   `obj/project.assets.json` перечисляют их по алфавиту. Сборки .NET
   загружаются в память при первом обращении к ним во время работы
   программы. Наша программа выводит топологический порядок — зависимости
   раньше зависящих от них пакетов.
3. **Версии.** NuGet применяет правила «direct dependency wins» (версия,
   указанная прямо в проекте, важнее транзитивной) и «cousin dependencies»
   (если два пакета требуют `Serilog >= 2.0` и `Serilog >= 3.0`, берётся
   3.0 — наибольшая из нижних границ). Наша программа берёт наименьшую
   версию по диапазону, встреченному первым, поэтому для общей зависимости
   версия может оказаться ниже, чем у NuGet. В проверенных пакетах этого не
   случилось — версии совпали.
4. **Циклы.** Для NuGet цикл — ошибка `NU1108`, восстановление пакетов
   прерывается. Наша программа сообщает о цикле и продолжает работу.

### Тестовый режим

С параметром `--test-mode` в `--repo` передаётся путь к файлу с описанием
графа. Пакеты называются большими латинскими буквами:

```
# комментарий
A: B C      пакет A зависит от B и C
B: D
C: D
D:          у D нет зависимостей
```

Пакет, упомянутый как зависимость, но не описанный отдельной строкой,
считается пакетом без зависимостей. Ошибки файла: строка без двоеточия,
имя не из больших латинских букв (с номером строки), пакет описан дважды,
анализируемого пакета нет в файле.

Готовые тестовые репозитории в `tests/data/`:

| Файл | Случай |
|---|---|
| `simple.txt` | общая зависимость (ромб): D нужен и B, и C |
| `deep.txt` | несколько уровней и общих зависимостей |
| `cycle.txt` | цикл A → B → C → A |
| `self_loop.txt` | пакет зависит сам от себя |
| `two_cycles.txt` | два независимых цикла |
| `bad_name.txt`, `bad_format.txt` | ошибки в файле |

## Функции и настройки

### Параметры командной строки

| Параметр | Обязательный | По умолчанию | Что задаёт |
|---|---|---|---|
| `--package ИМЯ` | да | — | имя анализируемого пакета |
| `--repo URL_ИЛИ_ПУТЬ` | нет | `https://api.nuget.org/v3/index.json` | URL репозитория NuGet или путь к файлу тестового репозитория |
| `--test-mode` | нет | выключен | режим работы с тестовым репозиторием |
| `--output ФАЙЛ` | нет | `graph.svg` | имя файла с изображением графа |
| `--ascii-tree` | нет | выключен | вывод зависимостей в виде ASCII-дерева |
| `--load-order` | нет | выключен | вывести порядок загрузки зависимостей вместо графа |

### Ошибки

Любая ошибка выводится с префиксом `error:`, программа завершается с кодом 1.

| Источник | Ошибки |
|---|---|
| параметры | нет `--package`; пустое или недопустимое имя; в тестовом режиме — не большие латинские буквы; неверный URL; нет файла тестового репозитория; `--output` не `.svg` или нет папки; неизвестный или повторный параметр |
| тестовый файл | строка без двоеточия; неверное имя пакета (с номером строки); пакет описан дважды; анализируемого пакета нет в файле |
| репозиторий NuGet | нет связи; пакет или версия не найдены; нет версии под диапазон; код ошибки сервера; повреждённый JSON или XML |

### Вывод результата

Без `--load-order` выводится граф: для каждого пакета (с выбранной версией) —
его прямые зависимости в порядке обхода, затем найденные циклы:

```
Dependency graph of A (4 packages):
  A -> B
  B -> C
  C -> A, D
  D -> (none)
Cycles:
  A -> B -> C -> A
```

## Сборка и запуск тестов

Нужны компилятор C++17 (`g++`; на Windows — MinGW-w64) и утилита `curl`
(для работы с сетью; тестам и тестовому режиму она не нужна).

```
./run.sh --package Serilog          # Linux, macOS: сборка в bin/ и запуск
run.bat --package Serilog           # Windows

./test.sh                           # unit-тесты, Linux, macOS
test.bat                            # unit-тесты, Windows
```

| Файл тестов | Что проверяет |
|---|---|
| `tests/test_config.cpp` | значения по умолчанию, все параметры (включая `--load-order`), ошибки каждого параметра |
| `tests/test_json.cpp`, `tests/test_xml.cpp` | разбор JSON и XML, ошибки в тексте |
| `tests/test_versions.cpp` | сравнение версий, пререлизы, последняя стабильная, диапазоны и наименьшая подходящая версия |
| `tests/test_nuspec.cpp` | группы платформ, старый формат, выбор группы |
| `tests/test_nuget.cpp` | индекс сервиса, коды ответа, ошибка связи, граф по данным NuGet: версии, диапазоны, регистр имён |
| `tests/test_graph.cpp` | транзитивные зависимости, порядок DFS, общая зависимость запрашивается один раз, цикл, петля, два цикла, цепочка из 200 000 пакетов |
| `tests/test_order.cpp` | **порядок загрузки: цепочка, ромб, корень последним, граф `deep.txt`, циклы (замыкающая связь пропущена), цепочка из 100 000 пакетов** |
| `tests/test_local_repo.cpp` | разбор файла тестового репозитория, ошибки с номером строки, графы из `tests/data` |
| `tests/test_app.cpp` | запуск приложения целиком: граф, **`--load-order` (с циклом и с версиями NuGet)**, ошибки, коды возврата |

Тесты не обращаются к интернету: сервер NuGet подменяется объектом
`FakeFetcher` (`tests/fake_nuget.h`), который отдаёт сохранённые ответы или
собирает `.nuspec` для заданного набора пакетов.

## Примеры использования

```
$ ./run.sh --package A --test-mode --repo tests/data/deep.txt
Dependency graph of A (8 packages):
  A -> B, C
  B -> D, E
  D -> G
  G -> (none)
  E -> G, H
  H -> G
  C -> E, F
  F -> H
Cycles: none

$ ./run.sh --package A --test-mode --repo tests/data/two_cycles.txt
Dependency graph of A (6 packages):
  A -> B, D
  B -> C
  C -> B
  D -> E
  E -> F
  F -> D
Cycles:
  B -> C -> B
  D -> E -> F -> D

$ ./run.sh --package A --test-mode --load-order --repo tests/data/deep.txt
Load order of A (dependencies first):
  1. G
  2. D
  3. H
  4. E
  5. B
  6. F
  7. C
  8. A

$ ./run.sh --package A --test-mode --load-order --repo tests/data/two_cycles.txt
Load order of A (dependencies first):
  1. C
  2. B
  3. F
  4. E
  5. D
  6. A
Note: cycle B -> C -> B - its edge C -> B was ignored
Note: cycle D -> E -> F -> D - its edge F -> D was ignored

$ ./run.sh --package Serilog.Sinks.Console --load-order
Load order of Serilog.Sinks.Console 6.1.1 (dependencies first):
  1. System.Buffers 4.5.1
  2. System.Numerics.Vectors 4.4.0
  3. System.Runtime.CompilerServices.Unsafe 6.0.0
  4. System.Memory 4.5.5
  5. System.Diagnostics.DiagnosticSource 8.0.1
  6. System.Threading.Tasks.Extensions 4.5.4
  7. System.Threading.Channels 8.0.0
  8. Serilog 4.0.0
  9. Serilog.Sinks.Console 6.1.1

$ ./run.sh --package Serilog.Sinks.Console
Dependency graph of Serilog.Sinks.Console 6.1.1 (9 packages):
  Serilog.Sinks.Console 6.1.1 -> Serilog
  Serilog 4.0.0 -> System.Diagnostics.DiagnosticSource, System.Threading.Channels
  System.Diagnostics.DiagnosticSource 8.0.1 -> System.Memory, System.Runtime.CompilerServices.Unsafe
  System.Memory 4.5.5 -> System.Buffers, System.Numerics.Vectors, System.Runtime.CompilerServices.Unsafe
  System.Buffers 4.5.1 -> (none)
  System.Numerics.Vectors 4.4.0 -> (none)
  System.Runtime.CompilerServices.Unsafe 6.0.0 -> (none)
  System.Threading.Channels 8.0.0 -> System.Threading.Tasks.Extensions
  System.Threading.Tasks.Extensions 4.5.4 -> System.Runtime.CompilerServices.Unsafe
Cycles: none

$ ./run.sh --package Z --test-mode --repo tests/data/simple.txt
error: package not found in test repository: Z

$ ./run.sh --package A --test-mode --repo tests/data/bad_name.txt
error: line 3: invalid package name 'b'

$ ./run.sh --package No.Such.Package.Xyz
error: package not found: No.Such.Package.Xyz
```

## Структура проекта

```
src/
  main.cpp            точка входа
  app.h/.cpp          запуск приложения, вывод результата
  config.h/.cpp       параметры командной строки и их проверка
  graph.h/.cpp        построение графа (DFS без рекурсии), циклы, порядок загрузки
  local_repo.h/.cpp   тестовый репозиторий из текстового файла
  nuget.h/.cpp        репозиторий NuGet, выбор версий (NuGetSource)
  nuspec.h/.cpp       зависимости из файла .nuspec
  versions.h/.cpp     сравнение версий и диапазоны
  http.h/.cpp         интерфейс Fetcher и загрузка через curl
  json.h/.cpp         разбор JSON
  xml.h/.cpp          разбор XML
tests/
  test.h              минимальный фреймворк unit-тестов
  test_main.cpp       запуск всех тестов
  helpers.h           запуск приложения и временные файлы в тестах
  fake_nuget.h        подмена сети в тестах
  test_*.cpp          unit-тесты
  data/nuget/         сохранённые ответы сервера NuGet
  data/*.txt          файлы тестовых репозиториев
run.sh, run.bat       сборка и запуск приложения
test.sh, test.bat     сборка и запуск тестов
```

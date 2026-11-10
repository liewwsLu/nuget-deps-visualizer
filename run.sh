#!/bin/sh
# Сборка и запуск приложения: ./run.sh --package Serilog
# Программа собирается заново при каждом запуске, поэтому после
# переключения на другой коммит запускается код именно этого коммита.
DIR="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$DIR/bin"
g++ -std=c++17 -O2 -Wall -Wextra -o "$DIR/bin/depviz" "$DIR"/src/*.cpp || exit 1
"$DIR/bin/depviz" "$@"

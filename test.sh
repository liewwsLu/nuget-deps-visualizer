#!/bin/sh
# Сборка и запуск unit-тестов: ./test.sh
DIR="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$DIR/bin"
g++ -std=c++17 -O1 -Wall -Wextra -DDEPVIZ_TESTS -I"$DIR/src" -o "$DIR/bin/tests" \
    "$DIR"/src/*.cpp "$DIR"/tests/*.cpp || exit 1
cd "$DIR" && ./bin/tests

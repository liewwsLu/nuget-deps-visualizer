@echo off
rem Build and run unit tests: test.bat
if not exist "%~dp0bin" mkdir "%~dp0bin"
g++ -std=c++17 -O1 -Wall -Wextra -DDEPVIZ_TESTS -I"%~dp0src" -o "%~dp0bin\tests.exe" "%~dp0src\*.cpp" "%~dp0tests\*.cpp" || exit /b 1
cd /d "%~dp0"
bin\tests.exe

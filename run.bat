@echo off
rem Build and run the application: run.bat --package Serilog
if not exist "%~dp0bin" mkdir "%~dp0bin"
g++ -std=c++17 -O2 -Wall -Wextra -o "%~dp0bin\depviz.exe" "%~dp0src\*.cpp" || exit /b 1
"%~dp0bin\depviz.exe" %*

@echo off
rem Configure (first time) and build into build\ninja. Usage: tools\build.cmd [target]
setlocal
set "ROOT=%~dp0.."
if not exist "%ROOT%\build\ninja\build.ninja" call "%~dp0vsdev.cmd" cmake -S "%ROOT%" -B "%ROOT%\build\ninja" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo || exit /b 1
if "%~1"=="" (call "%~dp0vsdev.cmd" cmake --build "%ROOT%\build\ninja") else (call "%~dp0vsdev.cmd" cmake --build "%ROOT%\build\ninja" --target %*)

@echo off
REM sha256.bat produces a file hash compatible with BSD
set "file=%~nx1"

for /f "usebackq tokens=1" %%h in (`cmake -E sha256sum %file%`) do set "hash=%%h"
echo:SHA256 (%file%) = %hash%

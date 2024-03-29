setlocal ENABLEDELAYEDEXPANSION
setlocal ENABLEEXTENSIONS
cd %BUILD_PATH%
call ..\ci\vs.bat

del ./test/default.profraw
set "files="
for /f "usebackq delims=" %%r IN (`dir . /s /b ^| findstr /i /e ".profraw"`) DO (
    echo:Found '%%r'
    set "files=!files! %%r"
)
llvm-profdata merge !files! -o collected.profdata

set "test_exes="
for /f "usebackq delims=" %%t IN (`dir . /s /b ^| findstr /i /r "[-a-zA-Z\.]*[tT]est.exe$"`) DO set "test_exes=!test_exes! --object %%t"
llvm-cov export --format=lcov --instr-profile collected.profdata !test_exes! > lcov.info

:: todo Coveralls have been changed to codecov, but there are no windows coverages...
:: @echo:Uploading to Coveralls %CIRRUS_BRANCH%/%CIRRUS_BUILD_ID%
:: @coveralls-lcov -t %COVERALLS_TOKEN% -b %CIRRUS_BRANCH% -s "cirrus-ci" --service-job-id=%CIRRUS_BUILD_ID% lcov.info

endlocal

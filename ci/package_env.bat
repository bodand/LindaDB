@REM Windows is fix, we don't need to emulate the `uname` part
cd %BUILD_PATH%
call vars.cmd
for /f "usebackq" %%a in (`winarch`) do echo:set "CG_PKG_NAME=cg3-checker-%CG_VER%-Windows-%%a" >> vars.cmd

cd %BUILD_PATH%
for /f "usebackq" %%v in (`type versioninfo.txt`) do echo:set "CG_VER=%%v" >> vars.cmd

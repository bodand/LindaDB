for /f "usebackq tokens=*" %%f in (`vswhere -latest -products * -property installationPath`) do set "vsdir=%%f"
"%vsdir%\Common7\Tools\vsdevcmd.bat" -arch=x64 -host_arch=x64

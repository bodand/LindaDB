Write-Host -ForegroundColor DarkMagenta "Make/1"
ninja -t clean
Measure-Command { make }

Write-Host -ForegroundColor DarkMagenta "Make/8"
ninja -t clean
Measure-Command { make -j 8 }

Write-Host -ForegroundColor DarkMagenta "Make/34"
ninja -t clean
Measure-Command { make -j 34 }

Write-Host -ForegroundColor DarkMagenta "Ninja/1"
ninja -t clean
Measure-Command { ninja -j 1 }

Write-Host -ForegroundColor DarkMagenta "Ninja/8"
ninja -t clean
Measure-Command { ninja -j 8 }

Write-Host -ForegroundColor DarkMagenta "Ninja/34"
ninja -t clean
Measure-Command { ninja -j 34 }

Write-Host -ForegroundColor DarkMagenta "lb/1"
ninja -t clean
Measure-Command { mpiexec -n 2 p:\LindaDB\_build-release-x64-clang\demo\build-sys\RelWithDebInfo\lb.exe }

Write-Host -ForegroundColor DarkMagenta "lb/8"
ninja -t clean
Measure-Command { mpiexec -n 9 p:\LindaDB\_build-release-x64-clang\demo\build-sys\RelWithDebInfo\lb.exe }

Write-Host -ForegroundColor DarkMagenta "lb/34 -- broken"
ninja -t clean
#Measure-Command { mpiexec -n 35 p:\LindaDB\_build-release-x64-clang\demo\build-sys\RelWithDebInfo\lb.exe }

$ErrorActionPreference = "Stop"

Write-Output "To run config"
build
cmake -G "Visual Studio 16 2019" -A x64 ..
Write-Output "Config done"

Write-Output "To run build"
cmake --build . --verbose --config Release --target ALL_BUILD
Write-Output "Build done"

Write-Output "To run exe"
.\Release\blas_demo.exe
Write-Output "Run exe done"

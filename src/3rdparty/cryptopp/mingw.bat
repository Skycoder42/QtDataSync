@echo off
setlocal

set PATH=C:\Qt\Tools\mingw530_32\bin;%PATH%;
set MAKEFLAGS=-j%NUMBER_OF_PROCESSORS%

set VERSION=5_6_5
set NAME=CRYPTOPP_%VERSION%
set SHA512SUM=abca8089e2d587f59c503d2d6412b3128d061784349c735f3ee46be1cb9e3d0d0fed9a9173765fa033eb2dc744e03810de45b8cc2f8ca1672a36e4123648ea44

set sDir=%~dp0
set tDir=%temp%\cryptopp

mkdir %tDir%
cd %tDir%

powershell -Command "Invoke-WebRequest https://github.com/weidai11/cryptopp/archive/%NAME%.zip -OutFile .\%NAME%.zip" || exit /B 1
:: TODO verify checksum
7z x .\%NAME%.zip || exit /B 1

set "xDir=%sDir:\=/%"

cd cryptopp-%NAME%
mingw32-make static > nul || exit /B 1
mingw32-make install PREFIX=%xDir% || exit /B 1

cd %sDir%
rmdir /s /q %tDir%

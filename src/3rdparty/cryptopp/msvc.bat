@echo off
setlocal

set VERSION=5_6_5
set NAME=CRYPTOPP_%VERSION%
set SHA512SUM=abca8089e2d587f59c503d2d6412b3128d061784349c735f3ee46be1cb9e3d0d0fed9a9173765fa033eb2dc744e03810de45b8cc2f8ca1672a36e4123648ea44

set sDir=%~dp0
set tDir=%temp%\cryptopp

mkdir %sDir%\lib
mkdir %tDir%
cd %tDir%

powershell -Command "Invoke-WebRequest https://github.com/weidai11/cryptopp/archive/%NAME%.zip -OutFile .\%NAME%.zip" || exit /B 1
:: TODO verify checksum
7z x .\%NAME%.zip || exit /B 1

cd cryptopp-%NAME%

call %VC_DIR% %VC_VARSALL% || exit /B 1
set PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\

start /wait devenv.exe /upgrade .\cryptlib.vcxproj
powershell -Command "(gc cryptlib.vcxproj) -replace '<RuntimeLibrary>MultiThreadedDebug<\/RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>' | Out-File -encoding ASCII cryptlib.vcxproj" || exit /B 1
powershell -Command "(gc cryptlib.vcxproj) -replace '<RuntimeLibrary>MultiThreaded<\/RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>' | Out-File -encoding ASCII cryptlib.vcxproj" || exit /B 1

echo
echo
type .\cryptlib.vcxproj
echo
echo

msbuild /t:Build /p:Configuration=Debug;Platform=%VC_ARCH% cryptlib.vcxproj || exit /B 1
cd %VC_ARCH%\Output\Debug
copy cryptlib.lib %sDir%\lib\cryptlibd.lib
:: d only in cryplib, because copy replace....
copy cryptlib.pdb %sDir%\lib\cryptlib.pdb
cd ..\..\..

msbuild /t:Build /p:Configuration=Release;Platform=%VC_ARCH% cryptlib.vcxproj || exit /B 1
cd %VC_ARCH%\Output\Release
copy cryptlib.lib %sDir%\lib\cryptlib.lib
cd ..\..\..

mkdir %sDir%\include\cryptopp
for %%I in (*.h) do copy %%I %sDir%\include\cryptopp\

cd %sDir%
rmdir /s /q %tDir%

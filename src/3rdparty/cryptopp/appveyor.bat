@echo off

set version=7_0_0

echo %PLATFORM% | findstr /C:"winrt_x64" > nul && (
	set pkgName=%PLATFORM:~10%_64
)
echo %PLATFORM% | findstr /C:"winrt_x86" > nul && (
	set pkgName=%PLATFORM:~10%
)
if "%PLATFORM%" == "mingw53_32" (
	set pkgName=mingw
)
if "%pkgName%" == "" (
	set pkgName=%PLATFORM%
)

powershell -Command "Invoke-WebRequest https://github.com/Skycoder42/ci-builds/releases/download/cryptopp_%version%/cryptopp_%version%_%pkgName%.zip -OutFile %temp%\cryptopp.zip" || exit /B 1
7z x %temp%\cryptopp.zip -o.\src\3rdparty || exit /B 1

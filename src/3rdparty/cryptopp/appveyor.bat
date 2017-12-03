if "%PLATFORM%" == "mingw53_32" (
	call %~dp0\mingw.bat || exit /B 1
) else (
	:: prepare vcvarsall
	if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2017" (
		set VC_DIR="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
	)
	if "%APPVEYOR_BUILD_WORKER_IMAGE%" == "Visual Studio 2015" (
		set VC_DIR="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
	)

	:: find the varsall parameters
	if "%PLATFORM%" == "msvc2017_64" (
		set VC_VARSALL=amd64
		set VC_ARCH=x64
	)
	if "%PLATFORM%" == "winrt_x64_msvc2017" (
		set VC_VARSALL=amd64
		set VC_ARCH=x64
	)
	if "%PLATFORM%" == "winrt_x86_msvc2017" (
		set VC_VARSALL=amd64_x86
		set VC_ARCH=Win32
	)
	if "%PLATFORM%" == "winrt_armv7_msvc2017" (
		set VC_VARSALL=amd64_arm
		set VC_ARCH=Arm
	)
	if "%PLATFORM%" == "msvc2015_64" (
		set VC_VARSALL=amd64
		set VC_ARCH=x64
	)
	if "%PLATFORM%" == "msvc2015" (
		set VC_VARSALL=amd64_x86
		set VC_ARCH=Win32
	)

	:: build
	call %~dp0\msvc.bat %VC_ARCH% || exit /B 1
)

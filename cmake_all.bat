mkdir _package

set X64=ON
call :build
set X64=OFF
call :build

copy LICENSE _package
copy README.md _package

mkdir _package\Shaders
copy Shaders _package\Shaders\
mkdir _package\Skyboxes
copy Skyboxes _package\Skyboxes\

cd _package
set FN=Foxotron_%date:~0,4%-%date:~5,2%-%date:~8,2%.zip
zip -r -9 %FN% *
move %FN% ../
cd ..

rmdir /s /q _package
goto :eof

REM --------------------- BUILD TIME -------------------------------

:build

set COMPILER=Visual Studio 16 2019
set ARCH=Win32
if not "%X64%"=="ON" goto skipvs
set ARCH=x64
:skipvs

set OUT_DIR=x86
set PLATFORM=W32
if not "%X64%"=="ON" goto skipme
set OUT_DIR=x64
set PLATFORM=W64
:skipme

mkdir build.vs16.%OUT_DIR%
cd build.vs16.%OUT_DIR%
cmake -DFOXOTRON_64BIT="%X64%" -G "%COMPILER%" -A "%ARCH%" ../
cmake --build . --config Release 
mkdir ..\_package\
del ..\_package\Foxotron_%PLATFORM%.exe
copy .\Release\Foxotron.exe ..\_package\Foxotron_%PLATFORM%.exe
cd ..
goto :eof

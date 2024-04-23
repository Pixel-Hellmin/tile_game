@echo off

setLocal

REM This build has to be run from vim in the root of the project

where /q cl || (
  echo ERROR: "cl" not found - please run this from the MSVC x64 native tools command prompt.
  exit /b 1
)

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)

REM TODO(Fermin): Test by moving glfw lib to the project
set CommonCompilerFlags= -nologo -Zi /EHsc /I ..\src\include 
set CommonLinkerFlags= /NODEFAULTLIB:MSVCRTD /NODEFAULTLIB:LIBCMT -incremental:no -opt:ref ..\libs\glfw3.lib opengl32.lib msvcrt.lib Gdi32.lib User32.lib shell32.lib

if not exist .\build mkdir .\build
pushd build

echo -------------------------------------
echo Building with MSVC
echo -------------------------------------

REM HEADS UP: This time counter might cause errors in other systems.
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set startdate=%%b-%%a-%%c)
for /f "tokens=1-3 delims=/:" %%a in ("%TIME%")  do (set starttime=%%a:%%b:%%c)
@echo Compilation started %startdate% at %starttime%
@echo:

REM del *.pbd > NUL 2> NUL
REM echo WAITING FOR PBD > lock.tmp
call cl %CommonCompilerFlags% ..\src\code\main.cpp  /link %CommonLinkerFlags% /OUT:main_debug.exe
call cl -O2 %CommonCompilerFlags% ..\src\code\main.cpp  /link %CommonLinkerFlags% /OUT:main_release.exe
REM del lock.tmp

for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set finishdate=%%b-%%a-%%c)
for /f "tokens=1-3 delims=/:" %%a in ("%TIME%") do (set finishtime=%%a:%%b:%%c)
@echo:
@echo Compilation finished %finishdate% at %finishtime%

popd





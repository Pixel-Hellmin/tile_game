@echo off

setLocal

REM This build has to be run from vim in the root of the project
REM Should I build asset_builder in debug or release?

where /q cl || (
  echo ERROR: "cl" not found - please run this from the MSVC x64 native tools command prompt.
  exit /b 1
)

if "%Platform%" neq "x64" (
    echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt.
    exit /b 1
)

REM TODO(Fermin): Test by moving glfw lib to the project
set CommonCompilerFlags= -nologo -Zi /EHsc /I src\include /Fobuild\ 
set CommonLinkerFlags= /NODEFAULTLIB:MSVCRTD /NODEFAULTLIB:LIBCMT -incremental:no -opt:ref libs\glfw3.lib opengl32.lib msvcrt.lib Gdi32.lib User32.lib shell32.lib

if not exist .\build mkdir .\build
REM pushd build

echo -------------------------------------
echo Building with MSVC
echo -------------------------------------

REM HEADS UP: This time counter might cause errors in other systems.
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set startdate=%%b-%%a-%%c)
for /f "tokens=1-3 delims=/:" %%a in ("%TIME%")  do (set starttime=%%a:%%b:%%c)
@echo Compilation started %startdate% at %starttime%
@echo:

REM delete pdbs each compilation since we make a new one each time we compile for live code editing and send output streams to NUL so it doesnt echo
del build\*.pdb > NUL 2> NUL

call cl %CommonCompilerFlags% /Febuild\asset_builder.exe src\code\asset_builder.cpp  /link %CommonLinkerFlags% 
call cl %CommonCompilerFlags% src\code\game.cpp -LD /link /IMPLIB:build\game.lib /OUT:build\game.dll -incremental:no -opt:ref -PDB:build\game_%date:~-8,2%%date:~-5,2%%date:~-2,2%_%time:~-11,2%%time:~-8,2%%time:~-5,2%%time:~-2,2%.pdb -EXPORT:game_update_and_render
call cl %CommonCompilerFlags% /Febuild\windows_main_debug.exe src\code\windows_main.cpp  /link %CommonLinkerFlags% 
call cl -O2 %CommonCompilerFlags% /Febuild\windows_main_release.exe src\code\windows_main.cpp  /link %CommonLinkerFlags% 
REM del lock.tmp

for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set finishdate=%%b-%%a-%%c)
for /f "tokens=1-3 delims=/:" %%a in ("%TIME%") do (set finishtime=%%a:%%b:%%c)
@echo:
@echo Compilation finished %finishdate% at %finishtime%

REM popd





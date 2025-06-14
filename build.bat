@echo off

SET TITLE=SQLitein
SET DEBUG=1
SET IFLAGS=/Iinclude
SET CFLAGS=/Fe%TITLE% /nologo /W4 /GR /EHa /Oi /fp:fast /FC /link /INCREMENTAL:NO /opt:ref /subsystem:windows /entry:WinMainCRTStartup user32.lib gdi32.lib opengl32.lib Comdlg32.lib winsqlite3.lib lib/cimgui.lib

where cl.exe > nul 2>&1
if %errorlevel% neq 0 (call setup.bat)

ctime.exe -begin compile_time.txt

if "%DEBUG%" == "1" (
  cl src/main.c %IFLAGS% /Od /MTd /Zi /Fm /DDEBUG=1 %CFLAGS%
) else (
  cl src/main.c %IFLAGS% /Ox /MT /DDEBUG=0 %CFLAGS%
)

ctime.exe -end compile_time.txt
del compile_time.txt > NUL

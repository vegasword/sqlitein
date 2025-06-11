@echo off
del /F *.exe *.ilk *.exp *.lib *.pch *.map *.obj *.ini > NUL
for %%f in (*.pdb) do @if not "%%~nxf"=="cimgui.pdb" del "%%f"

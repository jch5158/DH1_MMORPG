@echo off
xcopy /Y /D "%~1vcpkg_installed\%~3\%~3\tools\crashpad\crashpad_handler.exe" "%~2"
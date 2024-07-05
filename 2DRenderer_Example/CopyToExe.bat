@echo off

if "%~1"=="Debug" (
  set targetDll="2DRenderer_LibD"
) else (
  set targetDll="2DRenderer_Lib"
)

echo f | xcopy /f /y /s "../2DRenderer_Lib/x64/2DRenderer_Lib/%1/%targetDll%.dll" "./x64/2DRenderer_Example/%1/"

pause
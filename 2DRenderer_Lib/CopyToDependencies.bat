@echo off

:: Clear the target directory first, before copying
IF NOT EXIST "../Dependencies/2DRenderer/" (
	Echo Directory "../Dependencies/2DRenderer/" does not exist
	Echo Creating directory "../Dependencies/2DRenderer/"
	MKDIR "../Dependencies/2DRenderer/"
)

cd ../Dependencies/2DRenderer/
del * /S /Q

:: Re-direct back to the script folder
IF NOT EXIST "../../2DRenderer_Lib/" (
	Echo !! Critical !!
	Echo Directory "../../2DRenderer_Lib/" does not exist!
	exit /b
)

cd ../../2DRenderer_Lib/

:: Copy all header files into include directory
echo f | xcopy /f /y /s "./*.h" "../Dependencies/2DRenderer/include/"

:: Copy *.lib file to lib directory
IF EXIST "./x64/2DRenderer_Lib/%1.lib" (
	echo f | xcopy /f /y /s "./x64/2DRenderer_Lib/%1.lib" "../Dependencies/2DRenderer/lib/"
) ELSE Echo %1.lib does not exist

:: Copy *.dll file to lib directory
IF EXIST "./x64/2DRenderer_Lib/%1.lib" (
	echo f | xcopy /f /y /s "./x64/2DRenderer_Lib/%1.dll" "../Dependencies/2DRenderer/lib/"
) ELSE Echo %1.dll does not exist

pause
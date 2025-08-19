@echo off
REM Black Engine Project - Distribution Package Creator
REM This script creates a portable release package

echo Creating Black Engine Project portable package...

REM Build the project in Release mode
echo Building project in Release mode...
cmake --build build --config Release --target BlackEngineProject

REM Create release package directory
echo Creating release package directory...
if not exist "release-package" mkdir "release-package"

REM Clean previous package
del /Q "release-package\*.*" 2>nul
rmdir /S /Q "release-package\assets" 2>nul

REM Copy Release files
echo Copying Release files...
xcopy "build\bin\Release\*" "release-package\" /E /Y /Q

REM Create documentation
echo Creating README file...
echo Black Engine Project - Portable Release Package > "release-package\README.txt"
echo ============================================ >> "release-package\README.txt"
echo. >> "release-package\README.txt"
echo Contents: >> "release-package\README.txt"
echo - BlackEngineProject.exe: Main game executable >> "release-package\README.txt"
echo - TileMapEditor.exe: Map editor >> "release-package\README.txt"
echo - assets/: Game assets (sprites, audio, maps, etc.) >> "release-package\README.txt"
echo - *.dll files: Visual C++ Runtime libraries >> "release-package\README.txt"
echo. >> "release-package\README.txt"
echo Installation: >> "release-package\README.txt"
echo 1. Copy this folder to the target PC >> "release-package\README.txt"
echo 2. Run BlackEngineProject.exe to start the game >> "release-package\README.txt"
echo. >> "release-package\README.txt"
echo If you get DLL errors, install: >> "release-package\README.txt"
echo Microsoft Visual C++ Redistributable 2015-2022 >> "release-package\README.txt"
echo Download: https://aka.ms/vs/17/release/vc_redist.x64.exe >> "release-package\README.txt"

REM Create ZIP package
echo Creating ZIP package...
powershell "Compress-Archive -Path 'release-package\*' -DestinationPath 'BlackEngineProject-Portable.zip' -Force"

echo.
echo Package created successfully!
echo - Folder: release-package\
echo - ZIP file: BlackEngineProject-Portable.zip
echo.
echo Ready for distribution to other PCs.
pause

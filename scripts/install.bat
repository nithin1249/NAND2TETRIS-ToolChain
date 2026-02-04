@echo off
setlocal EnableDelayedExpansion

echo ==========================================
echo  Installing Jack Compiler (Windows)
echo ==========================================

:: 1. Configuration
set "SOURCE_DIR=%~dp0"
set "INSTALL_DIR=%USERPROFILE%\.jack_toolchain"

:: 2. Create Destination Folders
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
if not exist "%INSTALL_DIR%\bin" mkdir "%INSTALL_DIR%\bin"
if not exist "%INSTALL_DIR%\tools" mkdir "%INSTALL_DIR%\tools"
if not exist "%INSTALL_DIR%\os" mkdir "%INSTALL_DIR%\os"

:: 3. Install Executable
if exist "%SOURCE_DIR%bin\NAND2TETRIS_win.exe" (
    copy /Y "%SOURCE_DIR%bin\NAND2TETRIS_win.exe" "%INSTALL_DIR%\bin\jack.exe" >nul
    echo [OK] Installed jack.exe
) else (
    echo [ERROR] Could not find bin\NAND2TETRIS_win.exe
    echo         Make sure you unzipped the entire folder.
    pause
    exit /b 1
)

:: 4. Install Libraries & Tools
xcopy /Y /S /Q "%SOURCE_DIR%tools\*" "%INSTALL_DIR%\tools\" >nul
if exist "%SOURCE_DIR%os" (
    xcopy /Y /S /Q "%SOURCE_DIR%os\*" "%INSTALL_DIR%\os\" >nul
)

:: =========================================================
::  PYTHON & DEPENDENCY CHECK
:: =========================================================
echo.
echo [SETUP] Checking Python environment...

:: Check if python is in the PATH
python --version >nul 2>&1
if %errorlevel% equ 0 (
    echo    -> Python detected. Installing dependencies...

    :: Attempt to install libraries (silently)
    pip install -r "%INSTALL_DIR%\tools\requirements.txt" >nul 2>&1

    if !errorlevel! equ 0 (
        echo    [OK] Libraries installed successfully.
    ) else (
        echo    [WARN] Could not auto-install Python libraries.
        echo           You may need to run this command manually:
        echo           pip install -r "%INSTALL_DIR%\tools\requirements.txt"
    )
) else (
    echo    [!] WARNING: Python is NOT installed.
    echo        The core compiler (NAND2TETRIS_win.exe) will work fine.
    echo        However, the Visualizer tools will NOT work until you install Python.
    echo        Get it here: https://www.python.org/downloads/
)

:: =========================================================
::  MAKING IT "WORK LIKE GIT" (Permanent Access)
:: =========================================================
echo.
echo [SETUP] Configuring terminal commands...

:: STRATEGY 1: The "System-Wide" Install (Requires Admin)
(
    echo @echo off
    echo "%INSTALL_DIR%\bin\jack.exe" %%*
) > "%SystemRoot%\jack.cmd" 2>nul

if exist "%SystemRoot%\jack.cmd" (
    echo.
    echo    [SUCCESS] INSTALLED AS ADMINISTRATOR!
    echo    -------------------------------------
    echo    You can now open ANY terminal window (Cmd, PowerShell, VS Code)
    echo    and type 'jack' immediately.
    echo.
) else (
    :: STRATEGY 2: The "User-Only" Install (No Admin)
    echo    [NOTE] Admin rights not detected. Using User Profile method.

    setx PATH "%INSTALL_DIR%\bin;%PATH%" >nul

    echo.
    echo    [SUCCESS] INSTALLED FOR USER!
    echo    -----------------------------
    echo    The 'jack' command has been added to your settings.
    echo.
    echo    IMPORTANT: You must CLOSE ALL TERMINAL WINDOWS and open
    echo               a new one for this change to take effect.
)

echo ==========================================
echo  Done.
echo ==========================================
pause
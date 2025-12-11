taskkill /F /IM oop_backend.exe 2>nul
timeout /t 1 >nul
cd /d C:\Users\user\OneDrive\Desktop\OOP\backend
cmake --build build --config Release
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Backend compiled successfully!
    echo Starting backend...
    cd build\Release
    start /B oop_backend.exe
    timeout /t 3 >nul
    echo Backend running at http://localhost:8082
    cd /d C:\Users\user\OneDrive\Desktop\OOP
    python test_integration.py
) else (
    echo.
    echo Compilation failed!
    pause
)

@echo off
echo Stopping old backend...
taskkill /F /IM oop_backend.exe 2>nul
timeout /t 2 >nul

echo.
echo Building backend...
cd /d C:\Users\user\OneDrive\Desktop\OOP\backend
cmake --build build --config Release

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✅ Build successful!
    timeout /t 1 >nul
    
    echo Starting backend...
    cd build\Release
    start /B oop_backend.exe
    timeout /t 3 >nul
    
    echo ✅ Backend running at http://localhost:8082
    echo.
    echo Starting frontend...
    cd /d C:\Users\user\OneDrive\Desktop\OOP
    start http://localhost:8501
    streamlit run client.py
) else (
    echo.
    echo ❌ Build failed!
    pause
)

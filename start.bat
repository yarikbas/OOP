@echo off
echo ========================================
echo Fleet Manager - Startup Script
echo ========================================
echo.

echo Stopping old processes...
taskkill /F /IM oop_backend.exe 2>nul
taskkill /F /IM streamlit.exe 2>nul
timeout /t 1 >nul

echo.
echo Starting Backend...
start /D "backend\build\Release" oop_backend.exe
timeout /t 2 >nul

echo Starting Frontend...
start cmd /k "streamlit run frontend\fleet_manager.py"
timeout /t 5 >nul

echo.
echo ========================================
echo Servers Started!
echo ========================================
echo.
echo Backend:  http://127.0.0.1:8082
echo Frontend: http://localhost:8501
echo.
echo Opening browser...
start http://localhost:8501

echo.
echo Done! Browser opened.
echo.
pause

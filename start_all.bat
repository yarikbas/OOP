@echo off
echo Starting Fleet Manager...
echo.

REM Start backend
echo [1/2] Starting backend server...
cd /d "%~dp0backend\build\Release"
start /B oop_backend.exe
cd /d "%~dp0"
timeout /t 3 /nobreak >nul
echo      Backend: http://localhost:8082

REM Start frontend
echo [2/2] Starting frontend...
start http://localhost:8501
streamlit run client.py

pause

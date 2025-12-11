taskkill /F /IM oop_backend.exe 2>nul
timeout /t 1 >nul
cd /d C:\Users\user\OneDrive\Desktop\OOP\backend\build\Release
start /B oop_backend.exe
echo Backend started
timeout /t 3 >nul
cd /d C:\Users\user\OneDrive\Desktop\OOP
python test_integration.py

# Start Backend
Write-Host "Starting backend on http://localhost:8082 ..." -ForegroundColor Green
Start-Process -FilePath "c:\Users\user\OneDrive\Desktop\OOP\backend\build\Release\oop_backend.exe" -WorkingDirectory "c:\Users\user\OneDrive\Desktop\OOP\backend\build\Release"

Start-Sleep -Seconds 3

# Start Frontend
Write-Host "Starting frontend on http://localhost:8501 ..." -ForegroundColor Green
Set-Location "c:\Users\user\OneDrive\Desktop\OOP\frontend"
Start-Process powershell -ArgumentList "-NoExit", "-Command", "streamlit run fleet_manager.py"

Write-Host "`n✅ Servers started!" -ForegroundColor Cyan
Write-Host "🔧 Backend:  http://localhost:8082" -ForegroundColor Yellow
Write-Host "🌐 Frontend: http://localhost:8501" -ForegroundColor Yellow

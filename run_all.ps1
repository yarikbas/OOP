echo "Зупиняю старий backend..."
Get-Process -Name oop_backend -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

echo "Компілюю backend..."
Set-Location "C:\Users\user\OneDrive\Desktop\OOP\backend"
cmake --build build --config Release

if ($LASTEXITCODE -eq 0) {
    echo "`n✅ Компіляція успішна!"
    echo "Запускаю backend..."
    Set-Location "build\Release"
    Start-Process -FilePath ".\oop_backend.exe" -WindowStyle Hidden
    Start-Sleep -Seconds 3
    
    echo "✅ Backend працює на http://localhost:8082`n"
    echo "Тестую endpoints..."
    Set-Location "C:\Users\user\OneDrive\Desktop\OOP"
    python test_integration.py
} else {
    echo "`n❌ Помилка компіляції!"
}

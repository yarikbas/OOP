$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$backend  = Join-Path $root "backend"
$frontend = Join-Path $root "frontend"

$buildDir = Join-Path $backend "build\vs2022-x64"
$exe      = Join-Path $buildDir "Debug\oop_backend.exe"

Write-Host "== Stop old backend ==" -ForegroundColor Cyan
Get-Process oop_backend -ErrorAction SilentlyContinue | Stop-Process -Force

Write-Host "== Configure CMake ==" -ForegroundColor Cyan
cmake --preset "vs2022-x64" -S $backend -B $buildDir | Out-Host

Write-Host "== Build backend ==" -ForegroundColor Cyan
cmake --build $buildDir --config Debug --target oop_backend | Out-Host

if (!(Test-Path $exe)) {
    throw "Backend exe not found: $exe"
}

Write-Host "== Start backend ==" -ForegroundColor Cyan
Start-Process $exe -WorkingDirectory $backend

# --------- Streamlit ---------
$venvPy = Join-Path $frontend ".venv\Scripts\python.exe"
if (!(Test-Path $venvPy)) {
    throw "Venv python not found: $venvPy. Create venv in frontend first."
}

$env:FLEET_BASE_URL = "http://127.0.0.1:8082"

Write-Host "== Start Streamlit ==" -ForegroundColor Cyan
$mainApp = Join-Path $frontend "fleet_manager.py"
if (!(Test-Path $mainApp)) {
    throw "Streamlit entry file not found: $mainApp"
}

Start-Process $venvPy `
    -WorkingDirectory $frontend `
    -ArgumentList @("-m", "streamlit", "run", $mainApp)

Write-Host "`nDONE ✅" -ForegroundColor Green
Write-Host "Backend:  http://127.0.0.1:8082/health"
Write-Host "Frontend: Streamlit will open in browser (usually http://localhost:8501)"

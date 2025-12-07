# run_all.ps1
# One-shot запуск Fleet Manager (backend + Streamlit frontend)
# Працює з будь-якою БД. За замовчуванням БД НЕ видаляє.

$ErrorActionPreference = "Stop"

# ---- дозволяємо запуск скриптів тільки в межах цього процесу ----
try {
    Set-ExecutionPolicy -Scope Process Bypass -Force | Out-Null
} catch {}

# ---- UTF-8 для нормального виводу ----
try {
    [Console]::OutputEncoding = [System.Text.Encoding]::UTF8
    $OutputEncoding = [System.Text.Encoding]::UTF8
} catch {}

# ================== 0) КОНФІГ ==================
# Якщо запускаєш скрипт не з кореня - він спробує визначити ROOT сам.
$scriptRoot = $null
try { $scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path } catch {}

if ($scriptRoot -and (Test-Path (Join-Path $scriptRoot "backend"))) {
    $ROOT = $scriptRoot
} else {
    $ROOT = "$env:USERPROFILE\OneDrive\Desktop\OOP"
}

$BACKEND  = Join-Path $ROOT "backend"
$FRONTEND = Join-Path $ROOT "frontend"

# якщо хочеш повністю скинути БД – став $true
$RESET_DB = $false

# твій preset
$PRESET    = "vs2022-x64"
$BUILD_DIR = Join-Path $BACKEND "build\$PRESET"
$EXE       = Join-Path $BUILD_DIR "Debug\oop_backend.exe"
$DB_DIR    = Join-Path $BACKEND "data"
$DB        = Join-Path $DB_DIR "app.db"

# кандидати портів (під твої реальні історичні запуски)
$PORTS = @(8082, 8081, 8080, 8083)

# ================== helpers ==================
function Write-Step($msg) {
    Write-Host "`n== $msg ==" -ForegroundColor Cyan
}

function Wait-Backend($ports) {
    for ($i=0; $i -lt 40; $i++) {
        foreach ($p in $ports) {
            $baseTry = "http://127.0.0.1:$p"
            try {
                $h = Invoke-RestMethod -Method Get -Uri "$baseTry/health" -TimeoutSec 1
                if ($h -and $h.status -eq "ok") { return $baseTry }
            } catch {}
        }
        Start-Sleep -Milliseconds 300
    }
    return $null
}

# ================== 1) INFO ==================
Write-Host "ROOT:      $ROOT"
Write-Host "BACKEND:   $BACKEND"
Write-Host "FRONTEND:  $FRONTEND"
Write-Host "PRESET:    $PRESET"
Write-Host "RESET_DB:  $RESET_DB"
Write-Host "PORTS:     $($PORTS -join ', ')"
Write-Host "============================"

# ================== 2) STOP OLD BACKEND ==================
Write-Step "Stop old backend"
Get-Process oop_backend -ErrorAction SilentlyContinue | Stop-Process -Force

# ================== 3) DB (optional) ==================
Write-Step "DB"
if ($RESET_DB) {
    if (!(Test-Path $DB_DIR)) {
        New-Item -ItemType Directory -Force -Path $DB_DIR | Out-Null
    }
    Remove-Item $DB       -Force -ErrorAction SilentlyContinue
    Remove-Item "$DB-wal" -Force -ErrorAction SilentlyContinue
    Remove-Item "$DB-shm" -Force -ErrorAction SilentlyContinue
    Write-Host "DB removed" -ForegroundColor Green
} else {
    Write-Host "DB збережена (RESET_DB = false)"
}

# ================== 4) CONFIGURE CMAKE ==================
Write-Step "Configure CMake"
if (!(Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Force -Path $BUILD_DIR | Out-Null
}
cmake --preset $PRESET -S $BACKEND -B $BUILD_DIR | Out-Host

# ================== 5) BUILD BACKEND ==================
Write-Step "Build backend"
cmake --build $BUILD_DIR --config Debug --target oop_backend | Out-Host

if (!(Test-Path $EXE)) {
    throw "Backend exe not found: $EXE"
}

# ================== 6) START BACKEND ==================
Write-Step "Start backend"
Start-Process $EXE -WorkingDirectory $BACKEND | Out-Null

Write-Step "Wait /health (auto-detect port)"
$BASE = Wait-Backend $PORTS
if (-not $BASE) {
    throw "Backend health not reachable on ports: $($PORTS -join ', ')"
}

Write-Host "Backend OK: $BASE/health" -ForegroundColor Green

# ================== 7) FRONTEND (Streamlit) ==================
Write-Step "Frontend venv (no Activate.ps1 needed)"

if (!(Test-Path $FRONTEND)) {
    throw "Frontend folder not found: $FRONTEND"
}

$venvDir = Join-Path $FRONTEND ".venv"
$venvPy  = Join-Path $venvDir ".\Scripts\python.exe"

if (!(Test-Path $venvPy)) {
    Write-Host "Creating .venv..."
    Set-Location $FRONTEND
    python -m venv .venv
}

# оновимо pip тихо
try { & $venvPy -m pip install -U pip | Out-Null } catch {}

Write-Step "Install deps"
Set-Location $FRONTEND

if (Test-Path "requirements.txt") {
    & $venvPy -m pip install -r requirements.txt | Out-Host
} else {
    & $venvPy -m pip install streamlit pandas requests | Out-Host
}

Write-Step "Start Streamlit"
$env:FLEET_BASE_URL = $BASE

$mainApp = Join-Path $FRONTEND "fleet_manager.py"
if (!(Test-Path $mainApp)) {
    throw "Streamlit entry file not found: $mainApp"
}

Write-Host "Frontend will use: FLEET_BASE_URL=$BASE" -ForegroundColor Green
Write-Host "Running: $mainApp" -ForegroundColor Green

# варіант 1 (з логами у цьому ж вікні)
& $venvPy -m streamlit run $mainApp

# варіант 2 (якщо колись захочеш НЕ блокувати консоль) - розкоментуй:
# Start-Process $venvPy -WorkingDirectory $FRONTEND `
#   -ArgumentList @("-m","streamlit","run",$mainApp) | Out-Null

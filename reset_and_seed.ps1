# reset_and_seed.ps1
# One-shot: wipe app.db -> build/start backend -> seed rich demo data via REST
# Place this file in project root: ...\OOP\reset_and_seed.ps1

$ErrorActionPreference = "Stop"

# ---- Paths ----
$root     = $PSScriptRoot
$backend  = Join-Path $root "backend"
$frontend = Join-Path $root "frontend"

if (!(Test-Path $backend)) { throw "Backend folder not found: $backend" }
if (!(Test-Path $frontend)) { Write-Host "Frontend folder not found (ok for seeding): $frontend" -ForegroundColor Yellow }

$buildDir = Join-Path $backend "build\vs2022-x64"
$exe      = Join-Path $buildDir "Debug\oop_backend.exe"

$dbDir    = Join-Path $backend "data"
$db       = Join-Path $dbDir "app.db"

$base = "http://127.0.0.1:8082"

# ---- Helpers ----
function Write-Step($msg) {
    Write-Host "`n== $msg ==" -ForegroundColor Cyan
}

function Wait-Health {
    for ($i=0; $i -lt 40; $i++) {
        try {
            $h = Invoke-RestMethod -Method Get -Uri "$base/health" -TimeoutSec 2
            if ($h -and $h.status -eq "ok") { return $true }
        } catch {}
        Start-Sleep -Milliseconds 300
    }
    return $false
}

function Post-Json($path, $obj) {
    $json  = $obj | ConvertTo-Json -Depth 6
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($json)

    return Invoke-RestMethod `
        -Method Post `
        -Uri "$base$path" `
        -Body $bytes `
        -ContentType "application/json; charset=utf-8" `
        -TimeoutSec 5
}

# Some endpoints may not exist in your current backend version.
# We'll try/catch for safety to keep the script usable.
function Try-Post($path, $obj, $label) {
    try {
        $r = Post-Json $path $obj
        Write-Host "  + $label OK" -ForegroundColor Green
        return $r
    } catch {
        Write-Host "  ! $label SKIP/FAIL: $($_.Exception.Message)" -ForegroundColor Yellow
        return $null
    }
}

# ---- 1) Stop backend ----
Write-Step "Stop old backend"
Get-Process oop_backend -ErrorAction SilentlyContinue | Stop-Process -Force

# ---- 2) Remove DB ----
Write-Step "Remove DB"
if (!(Test-Path $dbDir)) {
    New-Item -ItemType Directory -Path $dbDir | Out-Null
}
Remove-Item $db -Force -ErrorAction SilentlyContinue
Remove-Item "$db-wal" -Force -ErrorAction SilentlyContinue
Remove-Item "$db-shm" -Force -ErrorAction SilentlyContinue

# ---- 3) Configure CMake ----
Write-Step "Configure CMake"
cmake --preset "vs2022-x64" -S $backend -B $buildDir | Out-Host

# ---- 4) Build backend ----
Write-Step "Build backend"
cmake --build $buildDir --config Debug --target oop_backend | Out-Host

if (!(Test-Path $exe)) {
    throw "Backend exe not found: $exe"
}

# ---- 5) Start backend ----
Write-Step "Start backend"
Start-Process $exe -WorkingDirectory $backend | Out-Null

# ---- 6) Wait health ----
Write-Step "Wait /health"
if (-not (Wait-Health)) {
    throw "Backend health not reachable at $base/health"
}
Write-Host "  Backend OK" -ForegroundColor Green

# ---- 7) Seed extra demo data ----
Write-Step "Seed rich demo data"

# 7.1 Ports (add MORE on top of default 5)
$extraPorts = @(
    @{ name="Singapore";  region="Asia";    lat=1.29027;   lon=103.851959 },
    @{ name="Santos";     region="America"; lat=-23.960833; lon=-46.333611 },
    @{ name="Cape Town";  region="Africa";  lat=-33.9249;  lon=18.4241 },
    @{ name="Busan";      region="Asia";    lat=35.1796;   lon=129.0756 },
    @{ name="Sydney";     region="Oceania"; lat=-33.8688;  lon=151.2093 }
)

foreach ($p in $extraPorts) {
    Try-Post "/api/ports" $p ("Port " + $p.name) | Out-Null
}

# 7.2 Ship types (extend defaults)
$extraTypes = @(
    @{ code="icebreaker"; name="Icebreaker"; description="Polar operations / ледокол" },
    @{ code="fishing";    name="Fishing";    description="Industrial fishing fleet" }
)

foreach ($t in $extraTypes) {
    Try-Post "/api/ship-types" $t ("ShipType " + $t.code) | Out-Null
}

# 7.3 Companies
$companies = @(
    @{ name="Black Sea Logistics" },
    @{ name="Nordic Research Group" },
    @{ name="Pacific Cargo Co" },
    @{ name="Atlantic Cruises" },
    @{ name="Harbor Services Union" }
)

$companyIds = @()
foreach ($c in $companies) {
    $r = Try-Post "/api/companies" $c ("Company " + $c.name)
    if ($r -and $r.id) { $companyIds += [int]$r.id }
}

# 7.4 People
# ranks mostly ASCII to avoid ??? if some encoding still misbehaves;
# 1-2 Ukrainian ranks to test UTF-8 path
$people = @(
    @{ full_name="Ivan Koval";          rank="Captain";          active=$true },
    @{ full_name="Olena Moroz";         rank="Chief Engineer";   active=$true },
    @{ full_name="Yaroslav Hrytsenko";  rank="Boatswain";        active=$true },
    @{ full_name="Sofiia Bondar";       rank="Research Lead";   active=$true },
    @{ full_name="Nazar Petrenko";      rank="Deck Cadet";      active=$true },
    @{ full_name="Daria Shevchenko";    rank="Medic";           active=$true },
    @{ full_name="Mykola Taran";        rank="Navigator";       active=$true },
    @{ full_name="Iryna Hnatiuk";       rank="Капітан";         active=$true },
    @{ full_name="Andrii Lis";          rank="Інженер";         active=$true },
    @{ full_name="Kateryna Poliak";     rank="Radio Officer";   active=$true },
    @{ full_name="Inactive Person A";  rank="Engineer";        active=$false },
    @{ full_name="Inactive Person B";  rank="Deckhand";        active=$false }
)

foreach ($p in $people) {
    Try-Post "/api/people" $p ("Person " + $p.full_name) | Out-Null
}

# 7.5 Link companies <-> ports (if endpoint exists)
# Based on your CompaniesController: POST /api/companies/{id}/ports with body {port_id, is_hq}
# We'll reference by known names; if your backend doesn't support this yet, it will just skip.

# We'll attempt to fetch current ports list to map names->id using REST raw
try {
    $portsNow = Invoke-RestMethod -Method Get -Uri "$base/api/ports" -TimeoutSec 5
    $portMap = @{}
    foreach ($pp in $portsNow) { $portMap[$pp.name] = [int]$pp.id }

    function Add-CompanyPort($companyId, $portName, $isHq) {
        if (-not $portMap.ContainsKey($portName)) { return }
        $body = @{ port_id = $portMap[$portName]; is_hq = $isHq }
        Try-Post ("/api/companies/{0}/ports" -f $companyId) $body ("CompanyPort c=$companyId -> $portName") | Out-Null
    }

    if ($companyIds.Count -ge 4) {
        Add-CompanyPort $companyIds[0] "Odessa"    $true
        Add-CompanyPort $companyIds[0] "Rotterdam" $false
        Add-CompanyPort $companyIds[0] "Hamburg"   $false

        Add-CompanyPort $companyIds[1] "Hamburg"   $true
        Add-CompanyPort $companyIds[1] "Rotterdam" $false

        Add-CompanyPort $companyIds[2] "Singapore" $true
        Add-CompanyPort $companyIds[2] "Shanghai"  $false
        Add-CompanyPort $companyIds[2] "Busan"     $false

        Add-CompanyPort $companyIds[3] "New York"  $true
        Add-CompanyPort $companyIds[3] "Cape Town" $false
        Add-CompanyPort $companyIds[3] "Sydney"    $false
    }
} catch {
    Write-Host "  ! Company-port linking skipped: $($_.Exception.Message)" -ForegroundColor Yellow
}

# 7.6 Ships (varied statuses + types)
# NOTE: your backend may already seed some ships automatically.
# We'll just add extra diverse ones.

try {
    $portsNow = Invoke-RestMethod -Method Get -Uri "$base/api/ports" -TimeoutSec 5
    $portMap = @{}
    foreach ($pp in $portsNow) { $portMap[$pp.name] = [int]$pp.id }

    $ships = @(
        @{ name="Dnipro Trader";    type="cargo";      country="Ukraine"; port_id=$portMap["Odessa"];    status="docked";    company_id=($companyIds | Select-Object -First 1) },
        @{ name="Aurora Explorer";  type="research";   country="Norway";  port_id=$portMap["Hamburg"];   status="docked";    company_id=($companyIds | Select-Object -Skip 1 -First 1) },
        @{ name="Polar Sentinel";   type="icebreaker"; country="Norway";  port_id=$portMap["Hamburg"];   status="loading";   company_id=($companyIds | Select-Object -Skip 1 -First 1) },
        @{ name="Sakura Carrier";   type="cargo";      country="Japan";   port_id=$portMap["Singapore"]; status="loading";   company_id=($companyIds | Select-Object -Skip 2 -First 1) },
        @{ name="Blue Fin One";     type="fishing";    country="Japan";   port_id=$portMap["Singapore"]; status="docked";    company_id=($companyIds | Select-Object -Skip 2 -First 1) },
        @{ name="Ocean Melody";     type="passenger";  country="UK";      port_id=$portMap["New York"];  status="unloading"; company_id=($companyIds | Select-Object -Skip 3 -First 1) },
        @{ name="Liberty Star II";  type="passenger";  country="USA";     port_id=$portMap["New York"];  status="departed";  company_id=($companyIds | Select-Object -Skip 3 -First 1) },
        @{ name="Cosco Titan";      type="tanker";     country="China";   port_id=$portMap["Shanghai"];  status="docked";    company_id=0 }
    )

    foreach ($s in $ships) {
        Try-Post "/api/ships" $s ("Ship " + $s.name) | Out-Null
    }
} catch {
    Write-Host "  ! Ships seeding partially skipped: $($_.Exception.Message)" -ForegroundColor Yellow
}

Write-Step "DONE"
Write-Host "Backend: $base/health" -ForegroundColor Green
Write-Host "Try:    $base/api/ports / ships / people / companies / ship-types" -ForegroundColor Green


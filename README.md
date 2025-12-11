# Fleet Manager — OOP Project (Enhanced Version)

A comprehensive fleet management system combining a C++ backend (REST API) with a Python Streamlit frontend. Manage ships, crews, ports, companies, cargo, voyages, schedules, weather, and financial data with a modern web interface.

## 🆕 New Features (2025 Update)

### Backend Enhancements
- ✅ **Cargo Management** - Track cargo shipments with weight, volume, value, status
- ✅ **Voyage History** - Record voyage details with fuel consumption, costs, revenues
- ✅ **Financial Tracking** - Detailed expense breakdown per voyage
- ✅ **Schedule Management** - Recurring departure schedules (weekly/biweekly/monthly)
- ✅ **Weather Data** - Weather conditions by port with API integration

### Frontend Additions
- 📦 **Cargo Management Page** - CRUD cargo, filter by ship/status, statistics
- 🛳️ **Voyage History Page** - Track voyages, profitability charts, fuel efficiency
- 📅 **Schedules Page** - Manage departure schedules, calendar view
- 🌤️ **Weather Page** - Current weather by port, historical charts
- 💰 **Financial Reports Page** - Expense breakdown, trends, Excel export

### New Utilities
- 🌍 **Weather API Integration** (`weather_service.py`) - Fetch real-time weather from OpenWeatherMap
- 📄 **PDF Report Generation** (`report_generator.py`) - Generate voyage reports and cargo manifests
- 🔄 **Auto Weather Updates** (`update_weather.py`) - Periodic weather data refresh

## Quick Start

### Prerequisites
- Windows 10+ with PowerShell 5.1
- Python 3.12+
- Visual C++ Build Tools (included with Visual Studio 2022)
- CMake 3.20+

### Step 1: Build Backend

```powershell
cd backend
cmake --build build --config Release
```

### Step 2: Install Frontend Dependencies

```powershell
pip install streamlit pandas requests plotly openpyxl reportlab
```

### Step 3: Start Backend

```powershell
cd backend\build\Release
.\oop_backend.exe
```

Backend: **http://127.0.0.1:8082**

### Step 4: Start Frontend

```powershell
cd C:\Users\user\OneDrive\Desktop\OOP
streamlit run client.py
```

Frontend: **http://127.0.0.1:8501**

### Step 5 (Optional): Auto Weather Updates

```powershell
python update_weather.py
```

## 📋 Features

### Core Features
✅ Full CRUD for Ships, People, Ports, Companies, Ship Types
✅ Crew assignment/unassignment with audit trail
✅ Persistent SQLite audit logs (timestamp, level, event type, entity)
✅ Real-time analytics dashboard (charts: event types, log levels, time-series)
✅ JSON & CSV export with token-based access control
✅ Responsive Streamlit UI with sidebar navigation and health banners

### New Features
📦 **Cargo Management** - Track shipments with complete metadata
🛳️ **Voyage Tracking** - Historical voyage data with performance metrics
💰 **Financial Reports** - Cost breakdown, profitability analysis, Excel export
📅 **Schedules** - Recurring departure schedules with calendar view
🌤️ **Weather Integration** - Real-time weather data for all ports
📄 **PDF Reports** - Generate voyage reports and cargo manifests

## 📡 Key API Endpoints

### Core Endpoints
- `GET /health` → Service health check
- `GET /api/people` → List personnel
- `GET /api/ships` → List ships
- `POST /api/crew/assign` → Assign crew to ship
- `POST /api/crew/end` → End crew assignment
- `GET /api/logs?level=INFO&limit=100&offset=0` → Query audit logs
- `GET /api/export?token=fleet-export-2025` → Full JSON export (requires token)
- `GET /api/logs.csv?token=fleet-export-2025` → CSV export (requires token)

### New Endpoints
- `GET /api/cargo` → List all cargo
- `GET /api/cargo/by-ship/{id}` → Cargo by ship
- `GET /api/cargo/by-status/{status}` → Cargo by status (pending/loaded/in_transit/delivered)
- `POST /api/cargo` → Create cargo
- `PUT /api/cargo/{id}` → Update cargo
- `DELETE /api/cargo/{id}` → Delete cargo
- `GET /api/voyages` → List all voyages
- `GET /api/voyages/by-ship/{id}` → Voyages by ship
- `POST /api/voyages` → Create voyage record
- `GET /api/voyages/expenses` → List all expenses
- `GET /api/voyages/expenses/by-voyage/{id}` → Expenses by voyage
- `POST /api/voyages/expenses` → Create expense record
- `GET /api/schedules` → List all schedules
- `GET /api/schedules/active` → Active schedules only
- `GET /api/schedules/by-ship/{id}` → Schedules by ship
- `POST /api/schedules` → Create schedule
- `PUT /api/schedules/{id}` → Update schedule
- `GET /api/weather` → List all weather data
- `GET /api/weather/latest` → Latest weather for all ports
- `GET /api/weather/by-port/{id}` → Weather by port
- `POST /api/weather` → Create weather record

## 🔐 Access Control

Export endpoints (`/api/export`, `/api/logs.csv`) require token auth:
- **Token:** `fleet-export-2025`
- **Methods:** Query param `?token=...` or Header `Authorization: Bearer ...`

Example:
```powershell
# Fails (401)
Invoke-RestMethod -Uri http://127.0.0.1:8082/api/export

# Succeeds (200)
Invoke-RestMethod -Uri 'http://127.0.0.1:8082/api/export?token=fleet-export-2025'
```

## 🖥️ Frontend Pages

1. **Ship Management** (`1_⚓_Кораблі.py`) - List, create, update, delete ships; dashboard charts
2. **Crew & People** (`2_👥_Люди.py`) - Manage personnel and crew assignments
3. **Company Management** (`3_🏢_Компанії.py`) - CRUD companies; link to ports
4. **Ports** (`4_🌊_Порти.py`) - Manage ports with coordinates
5. **Ship Types** (`5_🚢_Типи_Кораблів.py`) - Manage ship type models
6. **Logs & Analytics** (`6_📊_Логи.py`) - Query audit logs; time-series charts; CSV/JSON export
7. **Cargo Management** (`7_📦_Управління_Вантажами.py`) - ⭐ NEW - Track cargo shipments
8. **Voyage History** (`8_🛳️_Історія_Подорожей.py`) - ⭐ NEW - Voyage tracking with analytics
9. **Schedules** (`9_📅_Розклади.py`) - ⭐ NEW - Recurring departure schedules
10. **Weather** (`10_🌤️_Погода.py`) - ⭐ NEW - Weather conditions by port
11. **Financial Reports** (`11_💰_Фінанси.py`) - ⭐ NEW - Expense breakdown and reports

## 🏗️ Architecture

- **Backend:** C++ (Drogon) + SQLite + REST API
- **Frontend:** Python (Streamlit) + Pandas + Requests + Plotly
- **Database:** SQLite with 14 tables:
  - **Core:** people, ships, companies, ports, ship_types, crew_assignments, logs
  - **New:** cargo, voyage_records, voyage_expenses, schedules, weather_data
- **Audit Logging:** All mutations logged to `logs` table with UTC timestamp, level, event type, entity
- **Reports:** PDF generation with ReportLab, Excel export with openpyxl
- **Weather:** OpenWeatherMap API integration

## 🌍 Weather API Setup

To use weather features, get a free API key:

1. Register at [OpenWeatherMap](https://openweathermap.org/api)
2. Get your API key (free tier: 60 calls/minute)
3. Edit `weather_service.py` and replace:
   ```python
   API_KEY = "your_actual_api_key_here"
   ```
4. Run `python update_weather.py` to fetch weather for all ports

## 📄 Report Generation

Generate PDF reports:

```python
from report_generator import ReportGenerator

# Voyage report
voyage_data = {...}  # Get from API
pdf = ReportGenerator.create_voyage_report(voyage_data)

# Cargo manifest
cargo_list = [...]  # Get from API
pdf = ReportGenerator.create_cargo_manifest(cargo_list)
```

Reports include:
- Voyage details with financial summary
- Cargo manifests with weight/volume totals
- Professional formatting with tables and styling

## 🛠️ Troubleshooting

**Backend won't start:** Kill any running `oop_backend.exe` and rebuild
```powershell
Get-Process -Name oop_backend | Stop-Process -Force
cmake --build . --config Release
```

**Frontend import errors:** Ensure venv is activated and packages installed
```powershell
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

**Port 8082 already in use:** Change `listeners[0].port` in `backend/config.json`

## 📊 Development

### Adding an API Endpoint
1. Create `src/controllers/MyController.cpp` with handler method
2. Register in `CMakeLists.txt` under `SOURCES`
3. Rebuild: `cmake --build . --config Release`

### Adding a Streamlit Page
1. Create `frontend/pages/N_MyPage.py`
2. Use `common.py` helpers (`api_get`, `api_post`, `api_put`, `api_del`)
3. Add sidebar boilerplate with health check

## 📝 Technical Stack

| Layer | Technology |
|-------|-----------|
| Backend API | C++17, Drogon 1.9.11, JSON/SQLite |
| Database | SQLite3 (file-based) with 14 tables |
| Frontend UI | Python 3.12, Streamlit, Pandas, Plotly |
| Build | CMake + MSBuild (Windows) |
| Package Mgmt | vcpkg (C++ deps), pip (Python deps) |
| Reports | ReportLab (PDF), openpyxl (Excel) |
| Weather | OpenWeatherMap REST API |

## 📊 Database Schema

### Core Tables
- `people` - Personnel records
- `ships` - Ship registry
- `companies` - Shipping companies
- `ports` - Port locations with coordinates
- `ship_types` - Ship type models
- `crew_assignments` - Crew-to-ship assignments
- `logs` - Audit trail

### New Tables (2025 Update)
- `cargo` - Cargo shipments with weight, volume, value, status
- `voyage_records` - Historical voyage data
- `voyage_expenses` - Financial breakdown per voyage
- `schedules` - Recurring departure schedules
- `weather_data` - Weather conditions by port

**Last Updated:** January 2025  
**Status:** Production-ready with cargo management, financial tracking, weather integration, and PDF reports

---

## 🚀 Development Roadmap

### Completed ✅
- Core CRUD operations
- Audit logging system
- Real-time analytics dashboard
- Cargo management system
- Voyage history tracking
- Financial reporting with Excel export
- Schedule management
- Weather API integration
- PDF report generation

### Future Ideas 💡
- Mobile responsive design
- Multi-user authentication
- Real-time vessel tracking (GPS)
- Predictive maintenance alerts
- Route optimization
- Multi-language support (currently supports UK/EN)

---

**Developed with ❤️ using C++ and Python**

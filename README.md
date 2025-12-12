# Fleet Manager

Modern maritime fleet management system with C++ REST API backend (Drogon framework) and Python Streamlit frontend.

---

## Features

### Core Modules
- Ship Management - Full CRUD operations, ship types, status tracking, voyage departures
- Crew & People - Personnel database, crew assignments, role management
- Company Management - Shipping companies with fleet associations
- Port Management - Worldwide ports with GPS coordinates and regional classification
- Logs & Analytics - Complete audit trail stored in SQLite database

### Key Capabilities
- Real-time ship departure tracking with ETA calculations
- Automatic distance calculation using Haversine formula
- Crew assignment validation (requires captain for departure)
- Ship type classification (Container, Tanker, Bulk, Cruise, Military, Research, etc.)
- Multi-region support (Europe, Asia, Africa, Americas, Australia, Antarctica, Arctic)
- Company-port associations and fleet analytics
- Data import/export (CSV format)
- Real-time voyage tracking dashboard

---

## Technology Stack

### Backend
- Language: C++17
- Framework: Drogon 1.9.11
- Database: SQLite3
- Build System: CMake + vcpkg
- Server: HTTP REST API on port 8082

### Frontend
- Language: Python 3.14
- Framework: Streamlit 1.52.1
- Libraries: pandas, requests
- Server: Web UI on port 8501

---

## Requirements

- OS: Windows 10/11
- Python: 3.12 or later
- Visual Studio: 2022 with C++ tools
- CMake: 3.20 or later
- PowerShell: 5.1 or later

---

## Quick Start

### Option 1: Automated (Recommended)

```bash
start.bat
```

This will:
1. Stop any running instances
2. Start backend server (port 8082)
3. Start frontend server (port 8501)
4. Open browser automatically

### Option 2: Manual Setup

#### Backend
```powershell
cd backend\build\Release
.\oop_backend.exe
```

#### Frontend
```powershell
pip install -r requirements.txt
streamlit run frontend\fleet_manager.py
```

---

## Project Structure

```
OOP/
├── backend/
│   ├── src/               # C++ source files
│   ├── include/           # Header files
│   ├── build/Release/     # Compiled binaries
│   ├── data/              # Database storage
│   │   └── app.db        # SQLite database
│   └── CMakeLists.txt
├── frontend/
│   ├── fleet_manager.py   # Main app
│   ├── common.py          # Shared utilities
│   └── pages/             # Streamlit pages
│       ├── 2_Ship_Management.py
│       ├── 3_Crew_&_People.py
│       ├── 4_Company_Management.py
│       ├── 5_Admin_Data.py
│       └── 6_Logs_&_Analytics.py
├── requirements.txt
├── start.bat
└── README.md
```

---

## API Endpoints

### Health
- GET /health - Server status check

### Ships
- GET /api/ships - List all ships
- POST /api/ships - Create ship
- PUT /api/ships/{id} - Update ship
- DELETE /api/ships/{id} - Delete ship
- GET /api/ships/{id}/crew - Get ship crew

### People
- GET /api/people - List personnel
- POST /api/people - Create person
- PUT /api/people/{id} - Update person
- DELETE /api/people/{id} - Delete person

### Ports
- GET /api/ports - List ports
- POST /api/ports - Create port
- PUT /api/ports/{id} - Update port
- DELETE /api/ports/{id} - Delete port

### Companies
- GET /api/companies - List companies
- POST /api/companies - Create company
- PUT /api/companies/{id} - Update company
- DELETE /api/companies/{id} - Delete company

### Crew
- POST /api/crew/assign - Assign crew to ship
- DELETE /api/crew/remove - Remove crew from ship

### Ship Types
- GET /api/ship-types - List ship types
- POST /api/ship-types - Create ship type

### Logs
- GET /api/logs - Query audit logs
- GET /api/logs.csv - Export logs as CSV

---

## Database Schema

**Tables:**
- ships - Ship registry with type, status, location, company
- ports - Port database with coordinates
- people - Personnel records with ranks
- companies - Shipping companies
- ship_types - Ship model classifications
- crew_assignments - Ship-crew relationships
- company_ports - Company-port associations
- logs - System audit trail

---

## Features Overview

### Ship Departure System
- Validates captain presence before departure
- Calculates travel distance using geographic coordinates
- Computes ETA based on ship speed (knots)
- Updates ship status to "departed"
- Tracks voyage in real-time

### Crew Management
- Assign/remove crew members
- View crew by ship
- Track active assignments
- Multiple roles: Captain, Engineer, Researcher, Navigation Officer, etc.

### Data Import/Export
- Import ships from CSV
- Import ports from CSV
- Auto-geocoding with OpenStreetMap Nominatim
- Export all data to CSV format

### Analytics Dashboard
- Fleet statistics
- Company fleet sizes
- Port usage metrics
- Real-time voyage tracking

---

## Development

### Rebuild Backend
```powershell
cd backend
cmake --build build --config Release
```

### Database Location
```
backend/data/app.db
```

### View Logs
Access via frontend: Logs & Analytics page

---

## License

Educational project for Object-Oriented Programming course.

---

## Getting Started

Run start.bat and access the app at:
- Frontend: http://localhost:8501
- Backend API: http://127.0.0.1:8082

---

Built with C++ and Python
- `GET /api/cargo/by-ship/{id}` - Get cargo by ship
- `GET /api/cargo/by-status/{status}` - Get cargo by status (pending/loaded/in_transit/delivered)
- `POST /api/cargo` - Create cargo record
- `PUT /api/cargo/{id}` - Update cargo record
- `DELETE /api/cargo/{id}` - Delete cargo record

### Voyage Tracking
- `GET /api/voyages` - List all voyage records
- `GET /api/voyages/by-ship/{id}` - Get voyages by ship
- `POST /api/voyages` - Create voyage record
- `GET /api/voyages/expenses` - List all expenses
- `GET /api/voyages/expenses/by-voyage/{id}` - Get expenses by voyage
- `POST /api/voyages/expenses` - Create expense record

### Schedule Management
- `GET /api/schedules` - List all schedules
- `GET /api/schedules/active` - Get active schedules only
- `GET /api/schedules/by-ship/{id}` - Get schedules by ship
- `POST /api/schedules` - Create schedule
- `PUT /api/schedules/{id}` - Update schedule

### Weather Data
- `GET /api/weather` - List all weather records
- `GET /api/weather/latest` - Get latest weather for all ports
- `GET /api/weather/by-port/{id}` - Get weather data by port
- `POST /api/weather` - Create weather record

### Data Export
Export endpoints require authentication token:
- `GET /api/export?token=fleet-export-2025` - Full JSON export
- `GET /api/logs.csv?token=fleet-export-2025` - CSV export

Token can be provided as query parameter or Authorization header:
```powershell
# Query parameter
Invoke-RestMethod -Uri 'http://127.0.0.1:8082/api/export?token=fleet-export-2025'

# Authorization header
Invoke-RestMethod -Uri 'http://127.0.0.1:8082/api/export' -Headers @{Authorization="Bearer fleet-export-2025"}
```

## Architecture

### Technology Stack
- **Backend:** C++17, Drogon 1.9.11 framework, SQLite3 database, JSON API
- **Frontend:** Python 3.12, Streamlit web framework, Pandas, Plotly visualization
- **Build System:** CMake, MSBuild (Visual Studio)
- **Package Management:** vcpkg (C++ dependencies), pip (Python packages)
- **Additional Libraries:** ReportLab (PDF generation), openpyxl (Excel export), OpenWeatherMap API

### Database Schema
The system uses SQLite with 14 tables:

**Core Tables:**
- `people` - Personnel records
- `ships` - Ship registry with vessel details
- `companies` - Shipping company information
- `ports` - Port locations with geographic coordinates
- `ship_types` - Ship classification and specifications
- `crew_assignments` - Crew-to-ship assignment tracking
- `logs` - System audit trail

**Extended Tables:**
- `cargo` - Cargo shipments with weight, volume, value, and status tracking
- `voyage_records` - Historical voyage data with fuel consumption and costs
- `voyage_expenses` - Detailed financial breakdown per voyage
- `schedules` - Recurring departure schedule definitions
- `weather_data` - Weather conditions by port with timestamps
- `company_ports` - Many-to-many relationship between companies and ports

All database mutations are logged to the `logs` table with UTC timestamps, severity levels, event types, and entity references for comprehensive audit trails.

## Configuration

### Weather API Setup
To enable weather data features:

1. Register for a free API key at https://openweathermap.org/api
2. Edit `weather_service.py` and add your API key:
   ```python
   API_KEY = "your_actual_api_key_here"
   ```
3. Run the update script:
   ```powershell
   python update_weather.py
   ```

The free tier provides 60 API calls per minute.

### PDF Report Generation
Generate voyage reports and cargo manifests:

```python
from report_generator import ReportGenerator

# Generate voyage report
voyage_data = {...}  # Retrieve from API
pdf = ReportGenerator.create_voyage_report(voyage_data)

# Generate cargo manifest
cargo_list = [...]  # Retrieve from API
pdf = ReportGenerator.create_cargo_manifest(cargo_list)
```

Reports include formatted tables, financial summaries, and professional styling.

## Troubleshooting

### Backend Issues

**Backend process won't start:**
```powershell
# Kill existing process
Get-Process -Name oop_backend | Stop-Process -Force

# Rebuild
cd backend\build
cmake --build . --config Release
```

**Port 8082 already in use:**
Edit `backend/config.json` and change the port number in the listeners configuration.

### Frontend Issues

**Python import errors:**
```powershell
# Activate virtual environment
.\.venv\Scripts\Activate.ps1

# Reinstall dependencies
pip install -r requirements.txt
```

**Connection refused errors:**
Ensure the backend server is running before starting the frontend application.

## Development

### Adding API Endpoints

1. Create controller implementation in `backend/src/controllers/MyController.cpp`
2. Define controller interface in `backend/include/controllers/MyController.h`
3. Register the controller in `backend/CMakeLists.txt` under SOURCES
4. Rebuild the backend:
   ```powershell
   cd backend\build
   cmake --build . --config Release
   ```

### Adding Frontend Pages

1. Create new page in `frontend/pages/N_PageName.py`
2. Use helper functions from `frontend/common.py`:
   - `api_get()` - GET requests
   - `api_post()` - POST requests
   - `api_put()` - PUT requests
   - `api_del()` - DELETE requests
3. Include standard sidebar and health check components

### Project Structure

```
OOP/
├── backend/
│   ├── build/               # Build output directory
│   ├── include/            # Header files
│   │   ├── controllers/   # API endpoint handlers
│   │   ├── repos/         # Database repositories
│   │   ├── models/        # Data models
│   │   └── db/            # Database connection
│   ├── src/               # Implementation files
│   │   ├── controllers/
│   │   ├── repos/
│   │   ├── db/
│   │   └── main.cpp       # Application entry point
│   ├── CMakeLists.txt     # Build configuration
│   ├── config.json        # Server configuration
│   └── vcpkg.json         # C++ dependencies
├── frontend/
│   ├── pages/             # Streamlit pages
│   ├── common.py          # Shared utilities
│   └── fleet_manager.py   # Main application
├── data/
│   └── app.db             # SQLite database
├── requirements.txt       # Python dependencies
└── README.md
```

## Technical Specifications

| Component | Technology |
|-----------|-----------|
| Backend API | C++17, Drogon 1.9.11 |
| Database | SQLite3 file-based |
| Frontend | Python 3.12, Streamlit |
| Visualization | Plotly charts |
| Build System | CMake + MSBuild |
| Package Management | vcpkg, pip |
| PDF Generation | ReportLab |
| Excel Export | openpyxl |
| Weather API | OpenWeatherMap REST API |

## License

This project is provided as-is for educational and commercial use.

Last Updated: December 2025

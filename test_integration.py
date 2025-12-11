"""
Quick Integration Test
Tests all new API endpoints
"""

import requests
import json
from datetime import datetime

API_URL = "http://localhost:8082"

def test_endpoint(method, url, data=None):
    """Test a single endpoint"""
    try:
        if method == "GET":
            response = requests.get(url, timeout=2)
        elif method == "POST":
            response = requests.post(url, json=data, timeout=2)
        else:
            return f"❓ Unknown method: {method}"
        
        if response.status_code == 200:
            result = response.json()
            if isinstance(result, list):
                count = len(result)
            elif isinstance(result, dict) and 'value' in result:
                count = len(result['value']) if isinstance(result['value'], list) else 1
            else:
                count = 'OK'
            return f"✅ {response.status_code} - {count} items"
        else:
            return f"⚠️ {response.status_code}"
    except Exception as e:
        return f"❌ Error: {str(e)[:50]}"

def main():
    print("="*60)
    print("Fleet Manager - API Integration Test")
    print("="*60)
    
    # Check backend
    try:
        response = requests.get(f"{API_URL}/health", timeout=2)
        if response.status_code == 200:
            print("✅ Backend is running\n")
        else:
            print("❌ Backend returned error\n")
            return
    except:
        print("❌ Backend is not running!")
        print("Please start: backend\\build\\Release\\oop_backend.exe\n")
        return
    
    # Core endpoints
    print("Core Endpoints:")
    print("-" * 60)
    tests = [
        ("GET", f"{API_URL}/api/ships", None, "Ships"),
        ("GET", f"{API_URL}/api/people", None, "People"),
        ("GET", f"{API_URL}/api/ports", None, "Ports"),
        ("GET", f"{API_URL}/api/companies", None, "Companies"),
        ("GET", f"{API_URL}/api/ship-types", None, "Ship Types"),
    ]
    
    for method, url, data, name in tests:
        result = test_endpoint(method, url, data)
        print(f"  {name:20} {result}")
    
    # New endpoints
    print("\nNew Endpoints (Cargo, Voyages, Schedules, Weather):")
    print("-" * 60)
    tests = [
        ("GET", f"{API_URL}/api/cargo", None, "Cargo - All"),
        ("GET", f"{API_URL}/api/voyages", None, "Voyages - All"),
        ("GET", f"{API_URL}/api/schedules", None, "Schedules - All"),
        ("GET", f"{API_URL}/api/schedules/active", None, "Schedules - Active"),
        ("GET", f"{API_URL}/api/weather", None, "Weather - All"),
        ("GET", f"{API_URL}/api/weather/latest", None, "Weather - Latest"),
    ]
    
    for method, url, data, name in tests:
        result = test_endpoint(method, url, data)
        print(f"  {name:20} {result}")
    
    # Summary
    print("\n" + "="*60)
    print("Test Summary:")
    print("  - If you see ✅ 200 - all endpoints are working")
    print("  - '0 items' is normal for empty database")
    print("  - Use the Streamlit frontend to add data")
    print("="*60)
    
    # Database info
    print("\nDatabase Tables:")
    print("  Core: ships, people, ports, companies, ship_types, crew_assignments")
    print("  New:  cargo, voyage_records, voyage_expenses, schedules, weather_data")
    print("\nFrontend Pages:")
    print("  Run: streamlit run client.py")
    print("  URL: http://localhost:8501")
    print("\nDocumentation:")
    print("  README.md - Full setup guide")
    print("  SUMMARY_2025.md - Complete changelog")
    print("="*60)

if __name__ == "__main__":
    main()

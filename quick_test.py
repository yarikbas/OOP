import requests

base_url = "http://localhost:8082"

endpoints = ["/api/cargo", "/api/voyages", "/api/schedules", "/api/weather"]

for endpoint in endpoints:
    try:
        response = requests.get(base_url + endpoint, timeout=2)
        print(f"{endpoint}: {response.status_code} - {response.json()}")
    except Exception as e:
        print(f"{endpoint}: ERROR - {e}")

import requests
import subprocess
import time
import os

# Start backend
backend_path = r"C:\Users\user\OneDrive\Desktop\OOP\backend\build\Release\oop_backend.exe"
print(f"Starting backend: {backend_path}")

# Kill any existing backend
os.system("taskkill /F /IM oop_backend.exe 2>nul")
time.sleep(1)

# Start backend
proc = subprocess.Popen([backend_path], cwd=r"C:\Users\user\OneDrive\Desktop\OOP\backend\build\Release")
print(f"Backend started with PID: {proc.pid}")

# Wait for backend to start
time.sleep(5)

# Test new endpoints
base_url = "http://localhost:8082"

endpoints = [
    "/api/cargo",
    "/api/voyages",
    "/api/schedules",
    "/api/weather",
]

print("\n" + "="*60)
print("TESTING NEW ENDPOINTS")
print("="*60)

for endpoint in endpoints:
    try:
        url = base_url + endpoint
        response = requests.get(url, timeout=2)
        print(f"\n{endpoint}")
        print(f"  Status: {response.status_code}")
        if response.status_code == 200:
            data = response.json()
            print(f"  Data: {data if len(str(data)) < 100 else str(data)[:100] + '...'}")
        else:
            print(f"  Error: {response.text[:100] if response.text else 'No content'}")
    except Exception as e:
        print(f"\n{endpoint}")
        print(f"  Error: {e}")

print("\n" + "="*60)

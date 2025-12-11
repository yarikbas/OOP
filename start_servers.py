import subprocess
import time
import os

# Start backend
backend_path = r"c:\Users\user\OneDrive\Desktop\OOP\backend\build\Release\oop_backend.exe"
backend_dir = r"c:\Users\user\OneDrive\Desktop\OOP\backend\build\Release"

print("Starting backend on http://localhost:8082 ...")
backend_process = subprocess.Popen(
    [backend_path],
    cwd=backend_dir,
    creationflags=subprocess.CREATE_NEW_CONSOLE
)

time.sleep(2)

# Start frontend
frontend_dir = r"c:\Users\user\OneDrive\Desktop\OOP\frontend"
streamlit_script = r"c:\Users\user\OneDrive\Desktop\OOP\frontend\fleet_manager.py"

print("Starting frontend on http://localhost:8501 ...")
frontend_process = subprocess.Popen(
    ["streamlit", "run", streamlit_script],
    cwd=frontend_dir,
    creationflags=subprocess.CREATE_NEW_CONSOLE
)

print("\n✅ Servers started!")
print("🔧 Backend:  http://localhost:8082")
print("🌐 Frontend: http://localhost:8501")
print("\nPress Ctrl+C to stop both servers...")

try:
    backend_process.wait()
    frontend_process.wait()
except KeyboardInterrupt:
    print("\nStopping servers...")
    backend_process.terminate()
    frontend_process.terminate()

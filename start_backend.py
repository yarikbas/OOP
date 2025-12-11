import subprocess
import os
import time

backend_exe = r"C:\Users\user\OneDrive\Desktop\OOP\backend\build\Release\oop_backend.exe"
os.system("taskkill /F /IM oop_backend.exe 2>nul")
time.sleep(1)
subprocess.Popen(backend_exe, cwd=os.path.dirname(backend_exe))
print("Backend started")

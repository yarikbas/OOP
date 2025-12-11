"""
Automatic Weather Update Script
Run this periodically to fetch weather data for all ports
"""

import requests
import time
import sys
import os

sys.path.append(os.path.dirname(__file__))
from weather_service import WeatherService

API_URL = "http://localhost:8082"

def update_weather_for_all_ports():
    """Fetch and save weather data for all ports"""
    
    print("Fetching ports...")
    try:
        ports_response = requests.get(f"{API_URL}/api/ports", timeout=5)
        
        if ports_response.status_code != 200:
            print(f"Error fetching ports: {ports_response.status_code}")
            return False
        
        ports = ports_response.json().get('value', [])
        print(f"Found {len(ports)} ports")
        
        success_count = 0
        
        for port in ports:
            print(f"\nFetching weather for {port['name']} ({port['country']})...")
            
            if WeatherService.fetch_weather_for_port(port, API_URL):
                print(f"  ✅ Success")
                success_count += 1
            else:
                print(f"  ❌ Failed")
            
            # Rate limiting - don't spam the API
            time.sleep(1)
        
        print(f"\n{'='*50}")
        print(f"Weather update complete: {success_count}/{len(ports)} successful")
        return True
        
    except Exception as e:
        print(f"Error: {e}")
        return False


if __name__ == "__main__":
    print("Maritime Weather Update Service")
    print("=" * 50)
    
    # Check if backend is running
    try:
        response = requests.get(f"{API_URL}/api/ports", timeout=2)
        print("✅ Backend is running")
    except:
        print("❌ Backend is not running!")
        print("Please start the backend first")
        sys.exit(1)
    
    # Run update
    update_weather_for_all_ports()
    
    # Optional: Run continuously
    run_continuous = input("\nRun continuous updates? (y/n): ").lower() == 'y'
    
    if run_continuous:
        interval = int(input("Update interval in minutes (default 60): ") or "60")
        
        print(f"\nStarting continuous weather updates every {interval} minutes...")
        print("Press Ctrl+C to stop")
        
        try:
            while True:
                print(f"\n\n{'='*50}")
                print(f"Update at {time.strftime('%Y-%m-%d %H:%M:%S')}")
                print('='*50)
                
                update_weather_for_all_ports()
                
                print(f"\nNext update in {interval} minutes...")
                time.sleep(interval * 60)
                
        except KeyboardInterrupt:
            print("\n\nStopped by user")

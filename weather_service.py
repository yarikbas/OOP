"""
Weather API Integration Module
Fetches real-time weather data from OpenWeatherMap API
"""

import requests
import json
from datetime import datetime
from typing import Dict, Optional

class WeatherService:
    """Service for fetching weather data from OpenWeatherMap"""
    
    # Free API key (demo - should be in environment variable in production)
    API_KEY = "demo_key_replace_with_real"
    BASE_URL = "https://api.openweathermap.org/data/2.5/weather"
    
    @staticmethod
    def get_weather_by_coordinates(lat: float, lon: float) -> Optional[Dict]:
        """
        Fetch weather data by coordinates
        
        Args:
            lat: Latitude (-90 to 90)
            lon: Longitude (-180 to 180)
            
        Returns:
            Dictionary with weather data or None if failed
        """
        try:
            params = {
                'lat': lat,
                'lon': lon,
                'appid': WeatherService.API_KEY,
                'units': 'metric'  # Celsius
            }
            
            response = requests.get(WeatherService.BASE_URL, params=params, timeout=5)
            
            if response.status_code == 200:
                data = response.json()
                
                # Convert to our format
                return {
                    'temperature_c': data['main']['temp'],
                    'wind_speed_kmh': data['wind']['speed'] * 3.6,  # m/s to km/h
                    'wind_direction_deg': data['wind'].get('deg', 0),
                    'conditions': WeatherService._map_conditions(data['weather'][0]['main']),
                    'visibility_km': data.get('visibility', 10000) / 1000,  # meters to km
                    'wave_height_m': 0.0,  # Not provided by free API
                    'warnings': json.dumps([]),
                    'recorded_at': datetime.now().isoformat()
                }
            else:
                print(f"Weather API error: {response.status_code}")
                return None
                
        except Exception as e:
            print(f"Error fetching weather: {e}")
            return None
    
    @staticmethod
    def _map_conditions(openweather_condition: str) -> str:
        """Map OpenWeatherMap conditions to our format"""
        mapping = {
            'Clear': 'clear',
            'Clouds': 'cloudy',
            'Rain': 'rainy',
            'Drizzle': 'rainy',
            'Thunderstorm': 'stormy',
            'Snow': 'rainy',
            'Mist': 'cloudy',
            'Fog': 'cloudy',
        }
        return mapping.get(openweather_condition, 'cloudy')
    
    @staticmethod
    def fetch_weather_for_port(port_data: Dict, api_url: str) -> bool:
        """
        Fetch weather for a port and save to database
        
        Args:
            port_data: Port dictionary with id, latitude, longitude
            api_url: Backend API base URL
            
        Returns:
            True if successful, False otherwise
        """
        weather = WeatherService.get_weather_by_coordinates(
            port_data['latitude'],
            port_data['longitude']
        )
        
        if weather:
            weather['port_id'] = port_data['id']
            
            try:
                response = requests.post(f"{api_url}/api/weather", json=weather, timeout=5)
                return response.status_code in [200, 201]
            except Exception as e:
                print(f"Error saving weather: {e}")
                return False
        
        return False


if __name__ == "__main__":
    # Example usage
    print("Weather Service Demo")
    print("=" * 50)
    
    # Test coordinates (Odesa, Ukraine)
    lat, lon = 46.4825, 30.7233
    
    print(f"\nFetching weather for coordinates: {lat}, {lon}")
    weather = WeatherService.get_weather_by_coordinates(lat, lon)
    
    if weather:
        print("\nWeather Data:")
        print(json.dumps(weather, indent=2))
    else:
        print("\nFailed to fetch weather data")
        print("Note: You need a valid OpenWeatherMap API key")
        print("Get one at: https://openweathermap.org/api")

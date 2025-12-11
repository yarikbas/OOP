import streamlit as st
import requests
import pandas as pd
import plotly.express as px
from datetime import datetime, timedelta

st.set_page_config(page_title="Погодні Умови", page_icon="🌤️", layout="wide")

lang = st.session_state.get('lang', 'uk')
t = {
    'uk': {
        'title': '🌤️ Погодні Умови',
        'add_weather': 'Додати дані про погоду',
        'weather_by_port': 'Погода по портах',
        'port': 'Порт',
        'temperature': 'Температура (°C)',
        'wind_speed': 'Швидкість вітру (км/год)',
        'wind_direction': 'Напрямок вітру (град)',
        'conditions': 'Умови',
        'visibility': 'Видимість (км)',
        'wave_height': 'Висота хвиль (м)',
        'warnings': 'Попередження (JSON)',
        'clear': 'Ясно',
        'cloudy': 'Хмарно',
        'rainy': 'Дощ',
        'stormy': 'Шторм',
        'submit': 'Зберегти',
        'latest_weather': 'Остання погода',
        'history': 'Історія',
        'all_ports': 'Всі порти',
        'recorded_at': 'Дата запису',
    },
    'en': {
        'title': '🌤️ Weather Conditions',
        'add_weather': 'Add Weather Data',
        'weather_by_port': 'Weather by Port',
        'port': 'Port',
        'temperature': 'Temperature (°C)',
        'wind_speed': 'Wind Speed (km/h)',
        'wind_direction': 'Wind Direction (deg)',
        'conditions': 'Conditions',
        'visibility': 'Visibility (km)',
        'wave_height': 'Wave Height (m)',
        'warnings': 'Warnings (JSON)',
        'clear': 'Clear',
        'cloudy': 'Cloudy',
        'rainy': 'Rainy',
        'stormy': 'Stormy',
        'submit': 'Submit',
        'latest_weather': 'Latest Weather',
        'history': 'History',
        'all_ports': 'All Ports',
        'recorded_at': 'Recorded At',
    }
}[lang]

API_URL = "http://localhost:8082"

st.title(t['title'])

# Add new weather data
with st.expander(t['add_weather'], expanded=False):
    with st.form("add_weather"):
        col1, col2 = st.columns(2)
        
        with col1:
            ports_response = requests.get(f"{API_URL}/api/ports")
            ports = ports_response.json().get('value', []) if ports_response.status_code == 200 else []
            port_options = {f"{p['name']} ({p['country']})": p['id'] for p in ports}
            port = st.selectbox(t['port'], options=list(port_options.keys()))
            
            temperature = st.number_input(t['temperature'], min_value=-50.0, max_value=60.0, step=0.5)
            wind_speed = st.number_input(t['wind_speed'], min_value=0.0, step=1.0)
            wind_direction = st.number_input(t['wind_direction'], min_value=0, max_value=360, step=1)
        
        with col2:
            conditions = st.selectbox(t['conditions'], options=['clear', 'cloudy', 'rainy', 'stormy'],
                                     format_func=lambda x: {'clear': t['clear'], 'cloudy': t['cloudy'],
                                                           'rainy': t['rainy'], 'stormy': t['stormy']}[x])
            visibility = st.number_input(t['visibility'], min_value=0.0, step=0.5)
            wave_height = st.number_input(t['wave_height'], min_value=0.0, step=0.1)
            warnings = st.text_area(t['warnings'], value='[]')
        
        submitted = st.form_submit_button(t['submit'])
        if submitted:
            weather_data = {
                'port_id': port_options[port],
                'temperature_c': temperature,
                'wind_speed_kmh': wind_speed,
                'wind_direction_deg': wind_direction,
                'conditions': conditions,
                'visibility_km': visibility,
                'wave_height_m': wave_height,
                'warnings': warnings,
                'recorded_at': datetime.now().isoformat()
            }
            response = requests.post(f"{API_URL}/api/weather", json=weather_data)
            if response.status_code in [200, 201]:
                st.success('Дані про погоду успішно додано!' if lang == 'uk' else 'Weather data successfully added!')
                st.rerun()
            else:
                st.error(f"Помилка: {response.text}" if lang == 'uk' else f"Error: {response.text}")

# Latest weather by port
st.subheader(t['latest_weather'])

ports_response = requests.get(f"{API_URL}/api/ports")
ports = ports_response.json().get('value', []) if ports_response.status_code == 200 else []

port_filter = st.selectbox(t['port'], options=[t['all_ports']] + [f"{p['name']} ({p['country']})" for p in ports])

# Get weather data
if port_filter == t['all_ports']:
    weather_response = requests.get(f"{API_URL}/api/weather/latest")
else:
    port_id = next((p['id'] for p in ports if f"{p['name']} ({p['country']})" == port_filter), None)
    if port_id:
        weather_response = requests.get(f"{API_URL}/api/weather/by-port/{port_id}")
    else:
        weather_response = None

if weather_response and weather_response.status_code == 200:
    weather_data = weather_response.json().get('value', [])
    
    if weather_data:
        # Display weather cards
        cols = st.columns(min(len(weather_data), 3))
        
        for i, weather in enumerate(weather_data[:9]):  # Show max 9 cards
            with cols[i % 3]:
                port_name = next((p['name'] for p in ports if p['id'] == weather['port_id']), 'Unknown')
                
                # Weather icon
                icons = {'clear': '☀️', 'cloudy': '☁️', 'rainy': '🌧️', 'stormy': '⛈️'}
                icon = icons.get(weather['conditions'], '🌤️')
                
                st.markdown(f"### {icon} {port_name}")
                
                col1, col2 = st.columns(2)
                with col1:
                    st.metric('Температура' if lang == 'uk' else 'Temperature', 
                             f"{weather['temperature_c']}°C")
                    st.metric('Вітер' if lang == 'uk' else 'Wind', 
                             f"{weather['wind_speed_kmh']} км/год" if lang == 'uk' else f"{weather['wind_speed_kmh']} km/h")
                
                with col2:
                    st.metric('Видимість' if lang == 'uk' else 'Visibility', 
                             f"{weather['visibility_km']} км" if lang == 'uk' else f"{weather['visibility_km']} km")
                    st.metric('Хвилі' if lang == 'uk' else 'Waves', 
                             f"{weather['wave_height_m']} м" if lang == 'uk' else f"{weather['wave_height_m']} m")
                
                st.caption(f"🕐 {weather.get('recorded_at', 'N/A')}")
                st.divider()
        
        # History chart
        st.subheader(t['history'])
        
        df = pd.DataFrame(weather_data)
        ports_dict = {p['id']: f"{p['name']}" for p in ports}
        df['port_name'] = df['port_id'].map(ports_dict)
        
        # Temperature history
        col1, col2 = st.columns(2)
        
        with col1:
            st.markdown('**Температура за часом**' if lang == 'uk' else '**Temperature History**')
            if 'recorded_at' in df.columns:
                fig = px.line(df, x='recorded_at', y='temperature_c', color='port_name',
                             labels={'recorded_at': t['recorded_at'], 
                                    'temperature_c': t['temperature'],
                                    'port_name': t['port']})
                st.plotly_chart(fig, use_container_width=True)
        
        with col2:
            st.markdown('**Швидкість вітру**' if lang == 'uk' else '**Wind Speed**')
            if 'recorded_at' in df.columns:
                fig = px.line(df, x='recorded_at', y='wind_speed_kmh', color='port_name',
                             labels={'recorded_at': t['recorded_at'], 
                                    'wind_speed_kmh': t['wind_speed'],
                                    'port_name': t['port']})
                st.plotly_chart(fig, use_container_width=True)
        
        # Conditions distribution
        st.markdown('**Розподіл погодних умов**' if lang == 'uk' else '**Weather Conditions Distribution**')
        conditions_count = df['conditions'].value_counts().reset_index()
        conditions_count.columns = ['conditions', 'count']
        conditions_count['conditions_label'] = conditions_count['conditions'].map({
            'clear': t['clear'], 'cloudy': t['cloudy'], 
            'rainy': t['rainy'], 'stormy': t['stormy']
        })
        
        fig = px.pie(conditions_count, values='count', names='conditions_label')
        st.plotly_chart(fig, use_container_width=True)
        
    else:
        st.info('Немає даних про погоду' if lang == 'uk' else 'No weather data found')
else:
    st.error(f"Помилка: {weather_response.text if weather_response else 'No response'}" 
             if lang == 'uk' else f"Error: {weather_response.text if weather_response else 'No response'}")

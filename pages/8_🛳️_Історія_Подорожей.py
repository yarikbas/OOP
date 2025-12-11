import streamlit as st
import requests
import pandas as pd
from datetime import datetime
import plotly.express as px
import plotly.graph_objects as go
import sys
import os

# Add parent directory to path for imports
sys.path.append(os.path.dirname(os.path.dirname(__file__)))
from report_generator import ReportGenerator

st.set_page_config(page_title="Історія Подорожей", page_icon="🛳️", layout="wide")

lang = st.session_state.get('lang', 'uk')
t = {
    'uk': {
        'title': '🛳️ Історія Подорожей',
        'add_voyage': 'Додати нову подорож',
        'voyage_list': 'Список подорожей',
        'ship': 'Корабель',
        'origin': 'Відправлення',
        'destination': 'Призначення',
        'departed_at': 'Дата відправлення',
        'arrived_at': 'Дата прибуття',
        'duration_hours': 'Тривалість (год)',
        'distance_km': 'Відстань (км)',
        'fuel_consumed': 'Витрата палива (л)',
        'total_cost': 'Загальні витрати ($)',
        'total_revenue': 'Загальний дохід ($)',
        'cargo_list': 'Список вантажів (JSON)',
        'crew_list': 'Список екіпажу (JSON)',
        'weather_conditions': 'Погодні умови',
        'notes': 'Примітки',
        'filter_by_ship': 'Фільтр по кораблю',
        'all_ships': 'Всі кораблі',
        'submit': 'Зберегти',
        'profitability': 'Прибутковість',
        'fuel_efficiency': 'Паливна ефективність',
        'avg_duration': 'Середня тривалість',
        'total_voyages': 'Всього подорожей',
        'profit': 'Прибуток',
    },
    'en': {
        'title': '🛳️ Voyage History',
        'add_voyage': 'Add New Voyage',
        'voyage_list': 'Voyage List',
        'ship': 'Ship',
        'origin': 'Origin',
        'destination': 'Destination',
        'departed_at': 'Departure Date',
        'arrived_at': 'Arrival Date',
        'duration_hours': 'Duration (hours)',
        'distance_km': 'Distance (km)',
        'fuel_consumed': 'Fuel Consumed (L)',
        'total_cost': 'Total Cost ($)',
        'total_revenue': 'Total Revenue ($)',
        'cargo_list': 'Cargo List (JSON)',
        'crew_list': 'Crew List (JSON)',
        'weather_conditions': 'Weather Conditions',
        'notes': 'Notes',
        'filter_by_ship': 'Filter by Ship',
        'all_ships': 'All Ships',
        'submit': 'Submit',
        'profitability': 'Profitability',
        'fuel_efficiency': 'Fuel Efficiency',
        'avg_duration': 'Average Duration',
        'total_voyages': 'Total Voyages',
        'profit': 'Profit',
    }
}[lang]

API_URL = "http://localhost:8082"

st.title(t['title'])

# Add new voyage
with st.expander(t['add_voyage'], expanded=False):
    with st.form("add_voyage"):
        col1, col2 = st.columns(2)
        
        with col1:
            ships_response = requests.get(f"{API_URL}/api/ships")
            ships = ships_response.json().get('value', []) if ships_response.status_code == 200 else []
            ship_options = {s['name']: s['id'] for s in ships}
            ship = st.selectbox(t['ship'], options=list(ship_options.keys()))
            
            ports_response = requests.get(f"{API_URL}/api/ports")
            ports = ports_response.json().get('value', []) if ports_response.status_code == 200 else []
            port_options = {f"{p['name']} ({p['country']})": p['id'] for p in ports}
            
            origin = st.selectbox(t['origin'], options=list(port_options.keys()))
            destination = st.selectbox(t['destination'], options=list(port_options.keys()))
            
            departed = st.datetime_input(t['departed_at'])
            arrived = st.datetime_input(t['arrived_at'])
        
        with col2:
            duration = st.number_input(t['duration_hours'], min_value=0.0, step=0.5)
            distance = st.number_input(t['distance_km'], min_value=0.0, step=10.0)
            fuel = st.number_input(t['fuel_consumed'], min_value=0.0, step=100.0)
            cost = st.number_input(t['total_cost'], min_value=0.0, step=100.0)
            revenue = st.number_input(t['total_revenue'], min_value=0.0, step=100.0)
            
            weather = st.text_input(t['weather_conditions'])
            notes = st.text_area(t['notes'])
        
        submitted = st.form_submit_button(t['submit'])
        if submitted:
            voyage_data = {
                'ship_id': ship_options[ship],
                'origin_port_id': port_options[origin],
                'destination_port_id': port_options[destination],
                'departed_at': departed.isoformat(),
                'arrived_at': arrived.isoformat(),
                'duration_hours': duration,
                'distance_km': distance,
                'fuel_consumed_liters': fuel,
                'total_cost_usd': cost,
                'total_revenue_usd': revenue,
                'weather_conditions': weather,
                'notes': notes
            }
            response = requests.post(f"{API_URL}/api/voyages", json=voyage_data)
            if response.status_code in [200, 201]:
                st.success('Подорож успішно додана!' if lang == 'uk' else 'Voyage successfully added!')
                st.rerun()
            else:
                st.error(f"Помилка: {response.text}" if lang == 'uk' else f"Error: {response.text}")

# Filter
ship_filter = st.selectbox(t['filter_by_ship'], options=[t['all_ships']] + [s['name'] for s in ships])

# Get voyages
voyages_response = requests.get(f"{API_URL}/api/voyages")
if voyages_response.status_code == 200:
    voyages = voyages_response.json().get('value', [])
    
    # Apply filter
    if ship_filter != t['all_ships']:
        ship_id = next((s['id'] for s in ships if s['name'] == ship_filter), None)
        if ship_id:
            voyages_response = requests.get(f"{API_URL}/api/voyages/by-ship/{ship_id}")
            voyages = voyages_response.json().get('value', []) if voyages_response.status_code == 200 else []
    
    if voyages:
        df = pd.DataFrame(voyages)
        
        # Get names
        ports_dict = {p['id']: f"{p['name']}" for p in ports}
        ships_dict = {s['id']: s['name'] for s in ships}
        
        df['ship_name'] = df['ship_id'].map(ships_dict)
        df['origin_name'] = df['origin_port_id'].map(ports_dict)
        df['destination_name'] = df['destination_port_id'].map(ports_dict)
        df['profit'] = df['total_revenue_usd'] - df['total_cost_usd']
        
        # Display table
        display_df = df[['ship_name', 'origin_name', 'destination_name', 'departed_at', 
                        'duration_hours', 'distance_km', 'fuel_consumed_liters', 'profit']]
        st.dataframe(display_df, use_container_width=True)
        
        # Statistics
        col1, col2, col3, col4 = st.columns(4)
        col1.metric(t['total_voyages'], len(voyages))
        col2.metric(t['avg_duration'], f"{df['duration_hours'].mean():.1f} год" if lang == 'uk' else f"{df['duration_hours'].mean():.1f} hrs")
        col3.metric(t['total_cost'], f"${df['total_cost_usd'].sum():,.0f}")
        col4.metric(t['profit'], f"${df['profit'].sum():,.0f}")
        
        # Charts
        col1, col2 = st.columns(2)
        
        with col1:
            st.subheader(t['profitability'])
            profit_by_ship = df.groupby('ship_name')['profit'].sum().reset_index()
            fig = px.bar(profit_by_ship, x='ship_name', y='profit', 
                        labels={'ship_name': t['ship'], 'profit': t['profit']})
            st.plotly_chart(fig, use_container_width=True)
        
        with col2:
            st.subheader(t['fuel_efficiency'])
            df['fuel_per_km'] = df['fuel_consumed_liters'] / df['distance_km']
            fuel_eff = df.groupby('ship_name')['fuel_per_km'].mean().reset_index()
            fig = px.bar(fuel_eff, x='ship_name', y='fuel_per_km',
                        labels={'ship_name': t['ship'], 'fuel_per_km': 'л/км' if lang == 'uk' else 'L/km'})
            st.plotly_chart(fig, use_container_width=True)
    else:
        st.info('Немає подорожей' if lang == 'uk' else 'No voyages found')
else:
    st.error(f"Помилка: {voyages_response.text}" if lang == 'uk' else f"Error: {voyages_response.text}")

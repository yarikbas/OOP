import streamlit as st
import requests
import pandas as pd
from datetime import datetime

st.set_page_config(page_title="Управління Вантажами", page_icon="📦", layout="wide")

# Language
lang = st.session_state.get('lang', 'uk')
t = {
    'uk': {
        'title': '📦 Управління Вантажами',
        'add_cargo': 'Додати новий вантаж',
        'cargo_list': 'Список вантажів',
        'name': 'Назва',
        'type': 'Тип',
        'weight': 'Вага (тонн)',
        'volume': 'Об\'єм (м³)',
        'value': 'Вартість (USD)',
        'origin': 'Порт відправлення',
        'destination': 'Порт призначення',
        'status': 'Статус',
        'ship': 'Корабель',
        'loaded_at': 'Завантажено',
        'delivered_at': 'Доставлено',
        'notes': 'Примітки',
        'filter_by_ship': 'Фільтр по кораблю',
        'filter_by_status': 'Фільтр по статусу',
        'pending': 'Очікує',
        'loaded': 'Завантажено',
        'in_transit': 'В дорозі',
        'delivered': 'Доставлено',
        'all_ships': 'Всі кораблі',
        'all_statuses': 'Всі статуси',
        'submit': 'Зберегти',
        'delete': 'Видалити',
        'edit': 'Редагувати',
        'success': 'Вантаж успішно додано!',
        'error': 'Помилка',
    },
    'en': {
        'title': '📦 Cargo Management',
        'add_cargo': 'Add New Cargo',
        'cargo_list': 'Cargo List',
        'name': 'Name',
        'type': 'Type',
        'weight': 'Weight (tonnes)',
        'volume': 'Volume (m³)',
        'value': 'Value (USD)',
        'origin': 'Origin Port',
        'destination': 'Destination Port',
        'status': 'Status',
        'ship': 'Ship',
        'loaded_at': 'Loaded At',
        'delivered_at': 'Delivered At',
        'notes': 'Notes',
        'filter_by_ship': 'Filter by Ship',
        'filter_by_status': 'Filter by Status',
        'pending': 'Pending',
        'loaded': 'Loaded',
        'in_transit': 'In Transit',
        'delivered': 'Delivered',
        'all_ships': 'All Ships',
        'all_statuses': 'All Statuses',
        'submit': 'Submit',
        'delete': 'Delete',
        'edit': 'Edit',
        'success': 'Cargo successfully added!',
        'error': 'Error',
    }
}[lang]

API_URL = "http://localhost:8082"

st.title(t['title'])

# Add new cargo section
with st.expander(t['add_cargo'], expanded=False):
    with st.form("add_cargo"):
        col1, col2 = st.columns(2)
        
        with col1:
            name = st.text_input(t['name'])
            cargo_type = st.text_input(t['type'])
            weight = st.number_input(t['weight'], min_value=0.0, step=0.1)
            volume = st.number_input(t['volume'], min_value=0.0, step=0.1)
            value = st.number_input(t['value'], min_value=0.0, step=100.0)
        
        with col2:
            # Get ports
            ports_response = requests.get(f"{API_URL}/api/ports")
            ports = ports_response.json().get('value', []) if ports_response.status_code == 200 else []
            port_options = {f"{p['name']} ({p['country']})": p['id'] for p in ports}
            
            origin_port = st.selectbox(t['origin'], options=list(port_options.keys()))
            dest_port = st.selectbox(t['destination'], options=list(port_options.keys()))
            
            # Get ships
            ships_response = requests.get(f"{API_URL}/api/ships")
            ships = ships_response.json().get('value', []) if ships_response.status_code == 200 else []
            ship_options = {s['name']: s['id'] for s in ships}
            ship_options['None'] = None
            
            ship = st.selectbox(t['ship'], options=list(ship_options.keys()))
            
            status_options = ['pending', 'loaded', 'in_transit', 'delivered']
            status = st.selectbox(t['status'], options=status_options)
            
            notes = st.text_area(t['notes'])
        
        submitted = st.form_submit_button(t['submit'])
        if submitted:
            cargo_data = {
                'name': name,
                'type': cargo_type,
                'weight_tonnes': weight,
                'volume_m3': volume,
                'value_usd': value,
                'origin_port_id': port_options[origin_port],
                'destination_port_id': port_options[dest_port],
                'status': status,
                'ship_id': ship_options[ship],
                'notes': notes
            }
            response = requests.post(f"{API_URL}/api/cargo", json=cargo_data)
            if response.status_code in [200, 201]:
                st.success(t['success'])
                st.rerun()
            else:
                st.error(f"{t['error']}: {response.text}")

# Filters
col1, col2 = st.columns(2)
with col1:
    ships_response = requests.get(f"{API_URL}/api/ships")
    ships = ships_response.json().get('value', []) if ships_response.status_code == 200 else []
    ship_filter = st.selectbox(t['filter_by_ship'], options=[t['all_ships']] + [s['name'] for s in ships])

with col2:
    status_filter = st.selectbox(t['filter_by_status'], 
                                 options=[t['all_statuses'], 'pending', 'loaded', 'in_transit', 'delivered'])

# Get cargo list
cargo_response = requests.get(f"{API_URL}/api/cargo")
if cargo_response.status_code == 200:
    cargo_list = cargo_response.json().get('value', [])
    
    # Apply filters
    if ship_filter != t['all_ships']:
        ship_id = next((s['id'] for s in ships if s['name'] == ship_filter), None)
        if ship_id:
            cargo_response = requests.get(f"{API_URL}/api/cargo/by-ship/{ship_id}")
            cargo_list = cargo_response.json().get('value', []) if cargo_response.status_code == 200 else []
    
    if status_filter != t['all_statuses']:
        cargo_response = requests.get(f"{API_URL}/api/cargo/by-status/{status_filter}")
        cargo_list = cargo_response.json().get('value', []) if cargo_response.status_code == 200 else []
    
    if cargo_list:
        # Create dataframe
        df = pd.DataFrame(cargo_list)
        
        # Get port and ship names
        ports_dict = {p['id']: f"{p['name']} ({p['country']})" for p in ports}
        ships_dict = {s['id']: s['name'] for s in ships}
        
        df['origin'] = df['origin_port_id'].map(ports_dict)
        df['destination'] = df['destination_port_id'].map(ports_dict)
        df['ship_name'] = df['ship_id'].map(ships_dict).fillna('—')
        
        # Display table
        display_df = df[['name', 'type', 'weight_tonnes', 'volume_m3', 'value_usd', 
                        'origin', 'destination', 'status', 'ship_name']]
        st.dataframe(display_df, use_container_width=True)
        
        # Summary statistics
        st.subheader('Статистика' if lang == 'uk' else 'Statistics')
        col1, col2, col3, col4 = st.columns(4)
        col1.metric('Всього вантажів' if lang == 'uk' else 'Total Cargo', len(cargo_list))
        col2.metric('Загальна вага (т)' if lang == 'uk' else 'Total Weight (t)', f"{df['weight_tonnes'].sum():.1f}")
        col3.metric('Загальний об\'єм (м³)' if lang == 'uk' else 'Total Volume (m³)', f"{df['volume_m3'].sum():.1f}")
        col4.metric('Загальна вартість ($)' if lang == 'uk' else 'Total Value ($)', f"${df['value_usd'].sum():,.0f}")
    else:
        st.info('Немає вантажів' if lang == 'uk' else 'No cargo found')
else:
    st.error(f"{t['error']}: {cargo_response.text}")

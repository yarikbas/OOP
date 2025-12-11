import streamlit as st
import requests
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from datetime import datetime

st.set_page_config(page_title="Розклади", page_icon="📅", layout="wide")

lang = st.session_state.get('lang', 'uk')
t = {
    'uk': {
        'title': '📅 Розклади Відправлень',
        'add_schedule': 'Додати новий розклад',
        'schedule_list': 'Список розкладів',
        'ship': 'Корабель',
        'origin': 'Порт відправлення',
        'destination': 'Порт призначення',
        'route_name': 'Назва маршруту',
        'departure_day': 'День тижня (1-7)',
        'departure_time': 'Час відправлення (HH:MM)',
        'recurring': 'Періодичність',
        'weekly': 'Щотижня',
        'biweekly': 'Раз на 2 тижні',
        'monthly': 'Щомісяця',
        'is_active': 'Активний',
        'notes': 'Примітки',
        'filter_active': 'Тільки активні',
        'filter_all': 'Всі розклади',
        'submit': 'Зберегти',
        'toggle_status': 'Змінити статус',
        'active': 'Активний',
        'inactive': 'Неактивний',
        'monday': 'Понеділок',
        'tuesday': 'Вівторок',
        'wednesday': 'Середа',
        'thursday': 'Четвер',
        'friday': 'П\'ятниця',
        'saturday': 'Субота',
        'sunday': 'Неділя',
    },
    'en': {
        'title': '📅 Departure Schedules',
        'add_schedule': 'Add New Schedule',
        'schedule_list': 'Schedule List',
        'ship': 'Ship',
        'origin': 'Origin Port',
        'destination': 'Destination Port',
        'route_name': 'Route Name',
        'departure_day': 'Day of Week (1-7)',
        'departure_time': 'Departure Time (HH:MM)',
        'recurring': 'Recurring',
        'weekly': 'Weekly',
        'biweekly': 'Biweekly',
        'monthly': 'Monthly',
        'is_active': 'Active',
        'notes': 'Notes',
        'filter_active': 'Active Only',
        'filter_all': 'All Schedules',
        'submit': 'Submit',
        'toggle_status': 'Toggle Status',
        'active': 'Active',
        'inactive': 'Inactive',
        'monday': 'Monday',
        'tuesday': 'Tuesday',
        'wednesday': 'Wednesday',
        'thursday': 'Thursday',
        'friday': 'Friday',
        'saturday': 'Saturday',
        'sunday': 'Sunday',
    }
}[lang]

API_URL = "http://localhost:8082"

st.title(t['title'])

# Add new schedule
with st.expander(t['add_schedule'], expanded=False):
    with st.form("add_schedule"):
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
            route_name = st.text_input(t['route_name'])
        
        with col2:
            day_names = [t['monday'], t['tuesday'], t['wednesday'], t['thursday'], 
                        t['friday'], t['saturday'], t['sunday']]
            departure_day = st.selectbox(t['departure_day'], options=list(range(1, 8)),
                                        format_func=lambda x: f"{x} - {day_names[x-1]}")
            departure_time = st.text_input(t['departure_time'], value="10:00")
            recurring = st.selectbox(t['recurring'], options=['weekly', 'biweekly', 'monthly'],
                                    format_func=lambda x: {'weekly': t['weekly'], 
                                                          'biweekly': t['biweekly'], 
                                                          'monthly': t['monthly']}[x])
            is_active = st.checkbox(t['is_active'], value=True)
            notes = st.text_area(t['notes'])
        
        submitted = st.form_submit_button(t['submit'])
        if submitted:
            schedule_data = {
                'ship_id': ship_options[ship],
                'origin_port_id': port_options[origin],
                'destination_port_id': port_options[destination],
                'route_name': route_name,
                'departure_day_of_week': departure_day,
                'departure_time': departure_time,
                'recurring': recurring,
                'is_active': is_active,
                'notes': notes
            }
            response = requests.post(f"{API_URL}/api/schedules", json=schedule_data)
            if response.status_code in [200, 201]:
                st.success('Розклад успішно додано!' if lang == 'uk' else 'Schedule successfully added!')
                st.rerun()
            else:
                st.error(f"Помилка: {response.text}" if lang == 'uk' else f"Error: {response.text}")

# Filter
filter_option = st.radio('Фільтр' if lang == 'uk' else 'Filter', 
                        [t['filter_all'], t['filter_active']], horizontal=True)

# Get schedules
if filter_option == t['filter_active']:
    schedules_response = requests.get(f"{API_URL}/api/schedules/active")
else:
    schedules_response = requests.get(f"{API_URL}/api/schedules")

if schedules_response.status_code == 200:
    schedules = schedules_response.json().get('value', [])
    
    if schedules:
        df = pd.DataFrame(schedules)
        
        # Get names
        ports_dict = {p['id']: f"{p['name']}" for p in ports}
        ships_dict = {s['id']: s['name'] for s in ships}
        
        df['ship_name'] = df['ship_id'].map(ships_dict)
        df['origin_name'] = df['origin_port_id'].map(ports_dict)
        df['destination_name'] = df['destination_port_id'].map(ports_dict)
        df['status'] = df['is_active'].map({True: t['active'], False: t['inactive']})
        
        day_names_map = {
            1: t['monday'], 2: t['tuesday'], 3: t['wednesday'], 4: t['thursday'],
            5: t['friday'], 6: t['saturday'], 7: t['sunday']
        }
        df['day_name'] = df['departure_day_of_week'].map(day_names_map)
        
        # Display cards
        for _, schedule in df.iterrows():
            with st.container():
                col1, col2, col3, col4 = st.columns([3, 2, 1, 1])
                
                with col1:
                    st.markdown(f"**{schedule['route_name']}**")
                    st.text(f"{schedule['origin_name']} → {schedule['destination_name']}")
                
                with col2:
                    st.text(f"🚢 {schedule['ship_name']}")
                    st.text(f"📅 {schedule['day_name']} {schedule['departure_time']}")
                
                with col3:
                    recurring_icon = {'weekly': '📅', 'biweekly': '📆', 'monthly': '🗓️'}
                    st.text(recurring_icon.get(schedule['recurring'], '📅'))
                    st.text(schedule['recurring'])
                
                with col4:
                    status_color = "🟢" if schedule['is_active'] else "🔴"
                    st.text(f"{status_color} {schedule['status']}")
                    
                    if st.button(t['toggle_status'], key=f"toggle_{schedule['id']}"):
                        update_data = {**schedule.to_dict(), 'is_active': not schedule['is_active']}
                        response = requests.put(f"{API_URL}/api/schedules/{schedule['id']}", json=update_data)
                        if response.status_code == 200:
                            st.rerun()
                
                st.divider()
        
        # Summary
        st.subheader('Статистика' if lang == 'uk' else 'Statistics')
        col1, col2, col3 = st.columns(3)
        col1.metric('Всього розкладів' if lang == 'uk' else 'Total Schedules', len(schedules))
        col2.metric('Активних' if lang == 'uk' else 'Active', df['is_active'].sum())
        col3.metric('Неактивних' if lang == 'uk' else 'Inactive', (~df['is_active']).sum())
        
        # Chart: Departures by day
        st.subheader('Відправлення по днях тижня' if lang == 'uk' else 'Departures by Day of Week')
        departures_by_day = df.groupby('day_name').size().reset_index(name='count')
        fig = px.bar(departures_by_day, x='day_name', y='count',
                    labels={'day_name': 'День' if lang == 'uk' else 'Day', 
                           'count': 'Кількість' if lang == 'uk' else 'Count'})
        st.plotly_chart(fig, use_container_width=True)
    else:
        st.info('Немає розкладів' if lang == 'uk' else 'No schedules found')
else:
    st.error(f"Помилка: {schedules_response.text}" if lang == 'uk' else f"Error: {schedules_response.text}")

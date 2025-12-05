import streamlit as st
import requests
import pandas as pd

# --- НАЛАШТУВАННЯ ---
st.set_page_config(page_title="Fleet Commander", layout="wide", page_icon="⚓")
API_URL = "http://localhost:8081/api"

# --- ФУНКЦІЇ ЗАВАНТАЖЕННЯ ДАНИХ ---
def load_data():
    try:
        # 1. Запитуємо Порти (для координат)
        ports_res = requests.get(f"{API_URL}/ports")
        ports = ports_res.json() if ports_res.status_code == 200 else []
        
        # 2. Запитуємо Кораблі
        ships_res = requests.get(f"{API_URL}/ships")
        ships = ships_res.json() if ships_res.status_code == 200 else []
        
        return ports, ships
    except requests.exceptions.ConnectionError:
        st.error("🚨 Помилка з'єднання! Перевір, чи запущений C++ сервер (oop_backend.exe).")
        return [], []

# --- ОБРОБКА ДАНИХ ---
ports, ships = load_data()

if ports and ships:
    # Створюємо "Мапу портів" для швидкого пошуку координат: {id: {lat, lon, name}}
    ports_map = {p['id']: {'lat': p['lat'], 'lon': p['lon'], 'name': p['name']} for p in ports}

    # Додаємо координати до кораблів (щоб намалювати їх на карті)
    map_data = []
    for ship in ships:
        p_id = ship.get('port_id')
        if p_id in ports_map:
            port_info = ports_map[p_id]
            map_data.append({
                'name': ship['name'],
                'type': ship['type'],
                'country': ship['country'],
                'port': port_info['name'],
                'lat': port_info['lat'],
                'lon': port_info['lon'],
                'size': 100  # Розмір точки на карті
            })
    
    df_ships = pd.DataFrame(map_data)

    # --- ІНТЕРФЕЙС (UI) ---
    st.title("⚓ Fleet Manager: Global Strategy")

    col_map, col_list = st.columns([3, 2])

    with col_map:
        st.subheader("🗺️ Світова карта флоту")
        # Малюємо карту. color в форматі RGB (синій)
        st.map(df_ships, latitude='lat', longitude='lon', size='size', color='#0044ff')

    with col_list:
        st.subheader("📋 Активний склад флоту")
        
        # Фільтр по типу корабля
        ship_types = ["All"] + list(set(s['type'] for s in ships))
        filter_type = st.selectbox("Фільтр за типом:", ship_types)

        for ship in ships:
            if filter_type != "All" and ship['type'] != filter_type:
                continue
                
            # Визначаємо іконку
            icon = "🚢"
            if ship['type'] == 'Military': icon = "🚀"
            elif ship['type'] == 'Cargo': icon = "📦"
            
            # Картка корабля
            port_name = ports_map.get(ship['port_id'], {}).get('name', 'Unknown')
            with st.expander(f"{icon} {ship['name']} ({ship['country']})"):
                st.write(f"**Тип:** {ship['type']}")
                st.write(f"**Локація:** {port_name}")
                st.write(f"**Статус:** {ship['status']}")
                if st.button(f"Управління {ship['id']}", key=ship['id']):
                    st.toast(f"Функція управління для {ship['name']} скоро буде!")

    # Кнопка оновлення внизу
    if st.button("🔄 Оновити дані з сервера"):
        st.rerun()

else:
    st.warning("Немає даних для відображення. Запустіть сервер C++.")
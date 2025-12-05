import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Admin Data", page_icon="⚙️", layout="wide")
st.title("⚙️ Адміністрування Довідників")
st.caption("Тут можна керувати базовими сутностями: Портами та Типами Кораблів.")

if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

# ================== ЗАВАНТАЖЕННЯ ДАНИХ ==================
try:
    ports_df = api.get_ports()
    types_df = api.get_ship_types()
    
    port_map = api.get_name_map(ports_df)
    type_map = api.get_name_map(types_df, name_col='code')
    
except Exception as e:
    st.error(f"Не вдалося завантажити довідники: {e}")
    st.stop()
    
# ================== ТАБИ ==================
tab_ports, tab_types = st.tabs([
    "⚓ Управління Портами",
    "📋 Управління Типами Кораблів"
])

# ---------- УПРАВЛІННЯ ПОРТАМИ (CRUD) ----------
with tab_ports:
    st.subheader("Управління Портами")
    crud_tabs = st.tabs(["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"])
    
    with crud_tabs[0]:  # Список
        st.dataframe(api.df_1based(ports_df), use_container_width=True)

    with crud_tabs[1]:  # Створити
        with st.form("create_port_form"):
            name = st.text_input("Назва порту", placeholder="Odesa")
            region = st.text_input("Регіон", placeholder="Europe")
            lat = st.number_input("Широта (Lat)", value=46.48, format="%.6f")
            lon = st.number_input("Довгота (Lon)", value=30.72, format="%.6f")
            
            if st.form_submit_button("Створити порт"):
                if name and region:
                    api.api_post(
                        "/api/ports",
                        {"name": name, "region": region, "lat": lat, "lon": lon},
                        success_msg=f"Порт '{name}' створено."
                    )
                else:
                    st.error("Назва та Регіон є обов'язковими.")

    with crud_tabs[2]:  # Оновити
        if ports_df.empty:
            st.info("Немає портів для оновлення.")
        else:
            port_id_to_update = st.selectbox(
                "Оберіть порт для оновлення",
                ports_df['id'].tolist(),
                format_func=lambda x: port_map.get(x, "N/A"),
                key="port_update_select"
            )
            selected_port = ports_df[ports_df["id"] == port_id_to_update].iloc[0]
            
            with st.form("update_port_form"):
                st.write(f"Оновлення: {selected_port['name']}")
                new_name = st.text_input("Назва", value=selected_port['name'])
                new_region = st.text_input("Регіон", value=selected_port['region'])
                new_lat = st.number_input("Широта", value=float(selected_port['lat']), format="%.6f")
                new_lon = st.number_input("Довгота", value=float(selected_port['lon']), format="%.6f")
                
                if st.form_submit_button("Оновити порт"):
                    if new_name and new_region:
                        api.api_put(
                            f"/api/ports/{port_id_to_update}",
                            {"name": new_name, "region": new_region, "lat": new_lat, "lon": new_lon},
                            success_msg=f"Порт '{new_name}' оновлено."
                        )
                    else:
                        st.error("Назва та Регіон є обов'язковими.")

    with crud_tabs[3]:  # Видалити
        if ports_df.empty:
            st.info("Немає портів для видалення.")
        else:
            port_id_to_delete = st.selectbox(
                "Оберіть порт для видалення",
                ports_df['id'].tolist(),
                format_func=lambda x: port_map.get(x, "N/A"),
                key="port_delete_select"
            )
            port_name = port_map.get(port_id_to_delete, "N/A")
            
            st.warning("Видалення порту призведе до помилки, якщо до нього приписані кораблі!", icon="⚠️")
            if st.button(f"❌ Видалити '{port_name}'", type="primary"):
                api.api_del(
                    f"/api/ports/{port_id_to_delete}",
                    success_msg=f"Порт '{port_name}' видалено."
                )

# ---------- УПРАВЛІННЯ ТИПАМИ КОРАБЛІВ (CRUD) ----------
with tab_types:
    st.subheader("Управління Типами Кораблів")
    crud_tabs_types = st.tabs(["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"])
    
    with crud_tabs_types[0]:  # Список
        st.dataframe(api.df_1based(types_df), use_container_width=True)

    with crud_tabs_types[1]:  # Створити
        with st.form("create_type_form"):
            code = st.text_input("Код типу (унікальний)", placeholder="cargo_special")
            name = st.text_input("Назва типу", placeholder="Special Cargo")
            description = st.text_area("Опис", placeholder="Ships for special cargo")
            
            if st.form_submit_button("Створити тип"):
                if code and name:
                    api.api_post(
                        "/api/ship-types",
                        {"code": code, "name": name, "description": description},
                        success_msg=f"Тип '{name}' створено."
                    )
                else:
                    st.error("Код та Назва є обов'язковими.")

    with crud_tabs_types[2]:  # Оновити
        if types_df.empty:
            st.info("Немає типів для оновлення.")
        else:
            type_id_to_update = st.selectbox(
                "Оберіть тип для оновлення",
                types_df['id'].tolist(),
                format_func=lambda x: f"{types_df[types_df['id'] == x].iloc[0]['name']} (code={types_df[types_df['id'] == x].iloc[0]['code']})"
            )
            selected_type = types_df[types_df["id"] == type_id_to_update].iloc[0]
            
            with st.form("update_type_form"):
                st.write(f"Оновлення: {selected_type['name']}")
                # Код (PK) не можна міняти, але він потрібен для PUT
                st.text_input("Код", value=selected_type['code'], disabled=True)
                new_name = st.text_input("Назва", value=selected_type['name'])
                new_description = st.text_area("Опис", value=selected_type['description'])
                
                if st.form_submit_button("Оновити тип"):
                    if new_name:
                        api.api_put(
                            f"/api/ship-types/{type_id_to_update}",
                            {"code": selected_type['code'], "name": new_name, "description": new_description},
                            success_msg=f"Тип '{new_name}' оновлено."
                        )
                    else:
                        st.error("Назва є обов'язковою.")
                        
    with crud_tabs_types[3]:  # Видалити
        if types_df.empty:
            st.info("Немає типів для видалення.")
        else:
            type_id_to_delete = st.selectbox(
                "Оберіть тип для видалення",
                types_df['id'].tolist(),
                format_func=lambda x: f"{types_df[types_df['id'] == x].iloc[0]['name']} (id={x})"
            )
            type_name = types_df[types_df['id'] == type_id_to_delete].iloc[0]['name']
            
            st.warning("Видалення типу призведе до помилки, якщо існують кораблі цього типу!", icon="⚠️")
            if st.button(f"❌ Видалити '{type_name}'", type="primary"):
                api.api_del(
                    f"/api/ship-types/{type_id_to_delete}",
                    success_msg=f"Тип '{type_name}' видалено."
                )

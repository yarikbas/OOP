from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Admin Data", page_icon="⚙️", layout="wide")
st.title("⚙️ Адміністрування Довідників")
st.caption("Тут можна керувати базовими сутностями: Портами та Типами Кораблів.")

if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

# ================== LOAD ==================
try:
    ports_df = api.get_ports()
    types_df = api.get_ship_types()

    port_map = api.get_name_map(ports_df)
except Exception as e:
    st.error(f"Не вдалося завантажити довідники: {e}")
    st.stop()

# ================== STICKY MAIN TABS ==================
tab = api.sticky_tabs(
    ["⚓ Управління Портами", "📋 Управління Типами Кораблів"],
    "admin_main_tabs",
)

# ---------- ПОРТИ ----------
if tab == "⚓ Управління Портами":
    st.subheader("Управління Портами")

    crud = api.sticky_tabs(
        ["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"],
        "admin_ports_crud_tabs",
    )

    # Список
    if crud == "📋 Список":
        st.dataframe(api.df_1based(ports_df), use_container_width=True)

    # Створити
    elif crud == "➕ Створити":
        with st.form("create_port_form"):
            name = st.text_input("Назва порту", placeholder="Odesa", key="create_port_name")
            region = st.text_input("Регіон", placeholder="Europe", key="create_port_region")
            lat = st.number_input("Широта (Lat)", value=46.48, format="%.6f", key="create_port_lat")
            lon = st.number_input("Довгота (Lon)", value=30.72, format="%.6f", key="create_port_lon")

            if st.form_submit_button("Створити порт"):
                if name and region:
                    api.api_post(
                        "/api/ports",
                        {"name": name, "region": region, "lat": lat, "lon": lon},
                        success_msg=f"Порт '{name}' створено."
                    )
                else:
                    st.error("Назва та Регіон є обов'язковими.")

    # Оновити
    elif crud == "🛠️ Оновити":
        if ports_df.empty or "id" not in ports_df.columns:
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
                new_name = st.text_input("Назва", value=str(selected_port.get('name', "")), key="update_port_name")
                new_region = st.text_input("Регіон", value=str(selected_port.get('region', "")), key="update_port_region")
                new_lat = st.number_input("Широта", value=float(selected_port.get('lat', 0.0)), format="%.6f", key="update_port_lat")
                new_lon = st.number_input("Довгота", value=float(selected_port.get('lon', 0.0)), format="%.6f", key="update_port_lon")

                if st.form_submit_button("Оновити порт"):
                    if new_name and new_region:
                        api.api_put(
                            f"/api/ports/{port_id_to_update}",
                            {"name": new_name, "region": new_region, "lat": new_lat, "lon": new_lon},
                            success_msg=f"Порт '{new_name}' оновлено."
                        )
                    else:
                        st.error("Назва та Регіон є обов'язковими.")

    # Видалити
    elif crud == "❌ Видалити":
        if ports_df.empty or "id" not in ports_df.columns:
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
            if st.button(f"❌ Видалити '{port_name}'", type="primary", key="port_delete_btn"):
                api.api_del(
                    f"/api/ports/{port_id_to_delete}",
                    success_msg=f"Порт '{port_name}' видалено."
                )

# ---------- ТИПИ КОРАБЛІВ ----------
elif tab == "📋 Управління Типами Кораблів":
    st.subheader("Управління Типами Кораблів")

    crud = api.sticky_tabs(
        ["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"],
        "admin_types_crud_tabs",
    )

    # Список
    if crud == "📋 Список":
        st.dataframe(api.df_1based(types_df), use_container_width=True)

    # Створити
    elif crud == "➕ Створити":
        with st.form("create_type_form"):
            code = st.text_input("Код типу (унікальний)", placeholder="cargo_special", key="create_type_code")
            name = st.text_input("Назва типу", placeholder="Special Cargo", key="create_type_name")
            description = st.text_area("Опис", placeholder="Ships for special cargo", key="create_type_desc")

            if st.form_submit_button("Створити тип"):
                if code and name:
                    api.api_post(
                        "/api/ship-types",
                        {"code": code, "name": name, "description": description},
                        success_msg=f"Тип '{name}' створено."
                    )
                else:
                    st.error("Код та Назва є обов'язковими.")

    # Оновити
    elif crud == "🛠️ Оновити":
        if types_df.empty or "id" not in types_df.columns:
            st.info("Немає типів для оновлення.")
        else:
            def type_label(tid):
                row = types_df[types_df["id"] == tid]
                if row.empty:
                    return f"id={tid}"
                r = row.iloc[0]
                return f"{r.get('name','')} (code={r.get('code','')})"

            type_id_to_update = st.selectbox(
                "Оберіть тип для оновлення",
                types_df['id'].tolist(),
                format_func=type_label,
                key="type_update_select",
            )
            selected_type = types_df[types_df["id"] == type_id_to_update].iloc[0]

            with st.form("update_type_form"):
                st.text_input("Код", value=str(selected_type.get('code', "")), disabled=True)
                new_name = st.text_input("Назва", value=str(selected_type.get('name', "")), key="update_type_name")
                new_description = st.text_area("Опис", value=str(selected_type.get('description', "")), key="update_type_desc")

                if st.form_submit_button("Оновити тип"):
                    if new_name:
                        api.api_put(
                            f"/api/ship-types/{type_id_to_update}",
                            {
                                "code": str(selected_type.get('code', "")),
                                "name": new_name,
                                "description": new_description,
                            },
                            success_msg=f"Тип '{new_name}' оновлено."
                        )
                    else:
                        st.error("Назва є обов'язковою.")

    # Видалити
    elif crud == "❌ Видалити":
        if types_df.empty or "id" not in types_df.columns:
            st.info("Немає типів для видалення.")
        else:
            def type_label2(tid):
                row = types_df[types_df["id"] == tid]
                if row.empty:
                    return f"id={tid}"
                r = row.iloc[0]
                return f"{r.get('name','')} (id={tid})"

            type_id_to_delete = st.selectbox(
                "Оберіть тип для видалення",
                types_df['id'].tolist(),
                format_func=type_label2,
                key="type_delete_select"
            )
            row = types_df[types_df["id"] == type_id_to_delete].iloc[0]
            type_name = row.get("name", "")

            st.warning("Видалення типу призведе до помилки, якщо існують кораблі цього типу!", icon="⚠️")
            if st.button(f"❌ Видалити '{type_name}'", type="primary", key="type_delete_btn"):
                api.api_del(
                    f"/api/ship-types/{type_id_to_delete}",
                    success_msg=f"Тип '{type_name}' видалено."
                )

import streamlit as st
import pandas as pd
import common as api  # Наш спільний файл

st.set_page_config(page_title="Ship Management", page_icon="⚓", layout="wide")

# Повідомлення про успіх після дій (з common.api_*)
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

st.title("⚓ Управління Кораблями (CRUD)")

# ================== ЗАВАНТАЖЕННЯ ДОВІДНИКІВ ==================
try:
    ports_df = api.get_ports()
    types_df = api.get_ship_types()
    companies_df = api.get_companies()
    ships_df = api.get_ships()

    # Створюємо словники для селекторів
    port_map = api.get_name_map(ports_df)
    type_map = api.get_name_map(types_df, name_col='code')

    # company_map тільки якщо є нормальний id
    if not companies_df.empty and "id" in companies_df.columns:
        company_map = api.get_name_map(companies_df)
    else:
        company_map = {}
        if not companies_df.empty:
            st.caption("⚠️ Таблиця компаній не містить стовпчика 'id' — колонка 'company' у списку кораблів буде прихована.")

except Exception as e:
    st.error(f"Не вдалося завантажити довідники: {e}")
    st.stop()

# ================== ТАБИ ДЛЯ CRUD ==================
tab_list, tab_create, tab_actions, tab_delete = st.tabs([
    "📋 Список Кораблів",
    "➕ Створити Корабель",
    "🛠️ Дії / Оновлення",
    "❌ Видалити Корабель"
])

# ---------- СПИСОК КОРАБЛІВ ----------

with tab_list:
    st.subheader("Список усіх кораблів у флоті")
    if ships_df.empty:
        st.info("Кораблів не знайдено.")
    else:
        # "Розшифровуємо" ID-шники в назви для зручності
        ships_display = ships_df.copy()

        if not ports_df.empty and "port_id" in ships_display.columns:
            ships_display["port"] = ships_display["port_id"].map(port_map)

        if (
            not companies_df.empty
            and "company_id" in ships_display.columns
            and company_map
        ):
            ships_display["company"] = ships_display["company_id"].map(company_map)

        # 🔑 Головний фікс: показуємо тільки наявні колонки
        desired_cols = ["id", "name", "type", "status", "country", "port", "company"]
        display_cols = [c for c in desired_cols if c in ships_display.columns]

        if "company" not in ships_display.columns:
            st.caption("⚠️ Стовпчик 'company' недоступний (join із компаніями поки не налаштований або backend не повертає назви компаній).")

        st.dataframe(
            api.df_1based(ships_display[display_cols]),
            use_container_width=True
        )

# ---------- СТВОРИТИ КОРАБЕЛЬ ----------

with tab_create:
    st.subheader("➕ Створити новий корабель")

    if ports_df.empty or types_df.empty:
        st.warning("Неможливо створити корабель. Спочатку потрібно додати порти та типи кораблів на сторінці '⚙️ Admin'.")
    else:
        with st.form("create_ship_form"):
            st.write("Заповніть дані нового корабля:")

            name = st.text_input("Назва корабля", placeholder="Mriya")
            country = st.text_input("Країна реєстрації", placeholder="Ukraine")

            # Вибір типу (використовуємо 'code' як ID)
            type_options = types_df['code'].tolist()
            selected_type = st.selectbox("Тип корабля", type_options)

            # Вибір порту приписки
            port_options = ports_df['id'].tolist()
            selected_port_id = st.selectbox(
                "Порт приписки",
                port_options,
                format_func=lambda x: f"{port_map.get(x, 'Невідомий порт')} (id={x})"
            )

            # Вибір компанії (опціонально)
            if not companies_df.empty and "id" in companies_df.columns:
                company_options = [0] + companies_df['id'].tolist()
            else:
                company_options = [0]

            selected_company_id = st.selectbox(
                "Компанія-власник (0 = немає)",
                company_options,
                format_func=lambda x: f"{company_map.get(x, 'N/A')} (id={x})" if x != 0 else "N/A (id=0)"
            )

            submitted = st.form_submit_button("Створити")

            if submitted:
                if not name or not country:
                    st.error("Назва та Країна є обов'язковими.")
                else:
                    payload = {
                        "name": name,
                        "type": selected_type,
                        "country": country,
                        "port_id": int(selected_port_id),
                        "company_id": int(selected_company_id),
                        "status": "docked"  # Початковий статус
                    }
                    api.api_post(
                        "/api/ships",
                        payload,
                        success_msg=f"Корабель '{name}' успішно створено!"
                    )
                    # Після api_post буде st.rerun(), і список оновиться

# ---------- ДІЇ / ОНОВЛЕННЯ ----------

with tab_actions:
    st.subheader("🛠️ Оновити / Виконати дії над кораблем")

    if ships_df.empty:
        st.info("Немає кораблів для виконання дій.")
    else:
        ship_name_map = api.get_ship_name_map()
        ship_options = list(ship_name_map.keys())

        selected_ship_id = st.selectbox(
            "Оберіть корабель",
            ship_options,
            format_func=lambda x: ship_name_map.get(x, "N/A"),
            key="ship_action_select"
        )

        selected_ship = ships_df[ships_df["id"] == selected_ship_id].iloc[0]
        ship_type = str(selected_ship["type"]).lower()

        st.markdown("---")

        with st.form("update_ship_form"):
            st.subheader(f"Оновлення: {selected_ship['name']}")

            # --- Спільні дії (Переміщення / Статус / Компанія) ---
            st.markdown("**1. Переміщення / Статус / Компанія**")

            # Порт
            port_ids = ports_df['id'].tolist()
            current_port_id = int(selected_ship['port_id']) if not pd.isna(selected_ship['port_id']) else port_ids[0]
            port_index = port_ids.index(current_port_id) if current_port_id in port_ids else 0

            new_port_id = st.selectbox(
                "Перемістити у порт",
                port_ids,
                format_func=lambda x: f"{port_map.get(x, 'Невідомий порт')} (id={x})",
                index=port_index
            )

            # Статус
            new_status = st.text_input("Змінити статус", value=selected_ship['status'])

            # Компанія
            if not companies_df.empty and "id" in companies_df.columns:
                company_ids = [0] + companies_df['id'].tolist()
            else:
                company_ids = [0]

            current_company_id = int(selected_ship['company_id']) if not pd.isna(selected_ship['company_id']) else 0
            if current_company_id not in company_ids:
                company_ids.append(current_company_id)

            company_index = company_ids.index(current_company_id)

            new_company_id = st.selectbox(
                "Компанія-власник (0 = немає)",
                company_ids,
                index=company_index,
                format_func=lambda x: f"{company_map.get(x, 'N/A')} (id={x})" if x != 0 else "N/A (id=0)"
            )

            update_submitted = st.form_submit_button("🚢 Оновити (Порт / Статус / Компанія)")
            if update_submitted:
                payload = {
                    "port_id": int(new_port_id),
                    "status": new_status,
                    "company_id": int(new_company_id)
                }
                api.api_put(
                    f"/api/ships/{selected_ship_id}",
                    payload,
                    success_msg=f"Корабель '{selected_ship['name']}' оновлено."
                )

        st.markdown("---")
        st.subheader("2. Спеціальні дії за типом")

        # --- Спеціальні дії ---
        if ship_type == "military":
            st.markdown("#### Військовий корабель: Атака")
            ships_in_port = ships_df[
                (ships_df["port_id"] == selected_ship["port_id"]) &
                (ships_df["id"] != selected_ship_id)
            ]
            if ships_in_port.empty:
                st.info("Немає інших кораблів у цьому порту для атаки.")
            else:
                target_map = api.get_ship_name_map()
                target_id = st.selectbox(
                    "Вибери ціль",
                    ships_in_port['id'].tolist(),
                    format_func=lambda x: target_map.get(x, "N/A"),
                    key="military_target"
                )
                if st.button("🔥 Атакувати (status=destroyed)"):
                    api.api_put(
                        f"/api/ships/{target_id}",
                        {"status": "destroyed"},
                        success_msg=f"Корабель (id={target_id}) атаковано!"
                    )

        elif ship_type == "research":
            st.markdown("#### Дослідницький корабель")
            if st.button("🔬 Відправити на дослідження (status=research_mission)"):
                api.api_put(
                    f"/api/ships/{selected_ship_id}",
                    {"status": "research_mission"},
                    success_msg="Корабель відправлено на дослідження."
                )

        elif ship_type == "passenger":
            st.markdown("#### Пасажирський корабель")
            if st.button("🧳 Відправити у рейс (status=on_trip)"):
                api.api_put(
                    f"/api/ships/{selected_ship_id}",
                    {"status": "on_trip"},
                    success_msg="Корабель вирушив у пасажирський рейс."
                )

        elif ship_type == "cargo":
            st.markdown("#### Вантажний корабель")
            col1, col2 = st.columns(2)
            with col1:
                if st.button("📥 Завантажити (status=loading)"):
                    api.api_put(
                        f"/api/ships/{selected_ship_id}",
                        {"status": "loading"},
                        success_msg="Корабель у статусі 'loading'."
                    )
            with col2:
                if st.button("📤 Розвантажити (status=unloading)"):
                    api.api_put(
                        f"/api/ships/{selected_ship_id}",
                        {"status": "unloading"},
                        success_msg="Корабель у статусі 'unloading'."
                    )

        else:
            st.caption(f"Для цього типу корабля ('{ship_type}') спеціальних дій не налаштовано.")

# ---------- ВИДАЛИТИ КОРАБЕЛЬ ----------

with tab_delete:
    st.subheader("❌ Видалити корабель")
    st.warning("УВАГА! Ця дія є незворотною.", icon="⚠️")

    if ships_df.empty:
        st.info("Немає кораблів для видалення.")
    else:
        ship_name_map = api.get_ship_name_map()
        ship_options = list(ship_name_map.keys())

        selected_ship_id_del = st.selectbox(
            "Оберіть корабель для видалення",
            ship_options,
            format_func=lambda x: ship_name_map.get(x, "N/A"),
            key="ship_delete_select"
        )

        ship_name = ship_name_map.get(selected_ship_id_del, "N/A")

        if st.button(f"❌ Видалити корабель '{ship_name}'", type="primary"):
            api.api_del(
                f"/api/ships/{selected_ship_id_del}",
                success_msg=f"Корабель '{ship_name}' видалено."
            )

import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Ships Management", page_icon="🚢", layout="wide")
st.title("🚢 Управління кораблями")

# Показати останнє повідомлення про успіх, якщо є
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

# ================== ЗАВАНТАЖЕННЯ ДАНИХ ==================
try:
    ships_df     = api.get_ships()
    ports_df     = api.get_ports()
    companies_df = api.get_companies()
    types_df     = api.get_ship_types()
except Exception as e:
    st.error(f"Не вдалося завантажити дані з backend: {e}")
    st.stop()

# Мапи id -> name
port_map     = api.get_name_map(ports_df) if not ports_df.empty else {}
company_map  = api.get_name_map(companies_df) if not companies_df.empty else {}
ship_type_map = api.get_name_map(types_df) if not types_df.empty else {}

# Статуси кораблів (фіксований список)
SHIP_STATUS_OPTIONS = [
    ("docked",    "⚓ docked — у порту"),
    ("loading",   "⬆️ loading — завантажується"),
    ("unloading", "⬇️ unloading — розвантажується"),
    ("at_sea",    "🌊 at_sea — у плаванні"),
]

STATUS_VALUES  = [v for v, _ in SHIP_STATUS_OPTIONS]
STATUS_LABELS  = {v: label for v, label in SHIP_STATUS_OPTIONS}

def status_format(value: str) -> str:
    return STATUS_LABELS.get(value, value or "невідомо")

# ================== ТАБИ ==================
tab_list, tab_create, tab_update, tab_delete = st.tabs([
    "📋 Список кораблів",
    "➕ Створити корабель",
    "🛠️ Оновити / Перемістити",
    "❌ Видалити корабель",
])

# ---------- 1. СПИСОК КОРАБЛІВ ----------
with tab_list:
    st.subheader("📋 Всі кораблі")

    if ships_df.empty:
        st.info("Поки що немає жодного корабля.")
    else:
        view = ships_df.copy()

        # Людські назви порту та компанії
        if "port_id" in view.columns:
            def port_label(pid):
                if pd.isna(pid) or pid == 0:
                    return "🌊 У плаванні"
                return port_map.get(int(pid), f"port id={pid}")
            view["port"] = view["port_id"].map(port_label)

        if "company_id" in view.columns:
            def company_label(cid):
                if pd.isna(cid) or cid == 0:
                    return "—"
                return company_map.get(int(cid), f"company id={cid}")
            view["company"] = view["company_id"].map(company_label)

        if "status" in view.columns:
            view["status"] = view["status"].map(status_format)

        cols_order = []
        for col in ["id", "name", "type", "country", "status", "port", "company"]:
            if col in view.columns:
                cols_order.append(col)
        for col in view.columns:
            if col not in cols_order:
                cols_order.append(col)
        view = view[cols_order]

        st.dataframe(api.df_1based(view), width="stretch")

# ---------- 2. СТВОРИТИ КОРАБЕЛЬ ----------
with tab_create:
    st.subheader("➕ Створити новий корабель")

    with st.form("create_ship_form"):
        name = st.text_input("Назва корабля", placeholder="Mriya Sea")

        # Тип корабля
        if types_df.empty:
            ship_type = st.text_input("Тип корабля (текстом)", value="Cargo")
        else:
            type_codes = types_df["code"].tolist() if "code" in types_df.columns else types_df["name"].tolist()
            ship_type = st.selectbox(
                "Тип корабля",
                type_codes,
                index=0,
            )

        country = st.text_input("Країна приписки", value="Ukraine")

        # Порт (з опцією “У плаванні”)
        options_ports = [0]
        if not ports_df.empty and "id" in ports_df.columns:
            options_ports += ports_df["id"].astype(int).tolist()

        def port_option_label(x: int) -> str:
            if x == 0:
                return "🌊 У плаванні (без порту)"
            return port_map.get(x, f"port id={x}")

        selected_port_id = st.selectbox(
            "Початкове розташування",
            options_ports,
            format_func=port_option_label,
        )

        # Статус — випадаючий список
        selected_status = st.selectbox(
            "Початковий статус",
            STATUS_VALUES,
            format_func=status_format,
            index=0,
        )

        # Компанія-власник
        company_ids = [0]
        if not companies_df.empty and "id" in companies_df.columns:
            company_ids += companies_df["id"].astype(int).tolist()

        def company_option_label(x: int) -> str:
            if x == 0:
                return "— (без компанії)"
            return company_map.get(x, f"company id={x}")

        selected_company_id = st.selectbox(
            "Компанія-власник (0 = немає)",
            company_ids,
            format_func=company_option_label,
        )

        submitted = st.form_submit_button("Створити корабель")

        if submitted:
            if not name:
                st.error("Назва корабля є обов'язковою.")
            else:
                payload = {
                    "name":       name,
                    "type":       ship_type,
                    "country":    country,
                    "port_id":    int(selected_port_id),   # 0 -> У плаванні
                    "status":     selected_status,
                    "company_id": int(selected_company_id),
                }
                api.api_post(
                    "/api/ships",
                    payload,
                    success_msg=f"Корабель '{name}' створено."
                )

# ---------- 3. ОНОВИТИ / ПЕРЕМІСТИТИ ----------
with tab_update:
    st.subheader("🛠️ Оновити дані корабля, перемістити та змінити статус")

    if ships_df.empty:
        st.info("Немає кораблів для оновлення.")
    else:
        ship_ids = ships_df["id"].astype(int).tolist()

        def ship_label(sid: int) -> str:
            row = ships_df[ships_df["id"] == sid]
            if row.empty:
                return f"Ship id={sid}"
            r = row.iloc[0]
            return f"{r['name']} (id={sid}, type={r['type']})"

        selected_ship_id = st.selectbox(
            "Оберіть корабель",
            ship_ids,
            format_func=ship_label,
            key="ship_update_select",
        )

        ship_row = ships_df[ships_df["id"] == selected_ship_id].iloc[0]

        st.markdown(f"**Оновлення: {ship_row['name']}**")
        st.markdown("**1. Переміщення / Статус / Компанія**")

        with st.form("update_ship_form"):
            # === Переміщення у порт (з опцією “У плаванні”) ===
            options_ports = [0]
            if not ports_df.empty and "id" in ports_df.columns:
                options_ports += ports_df["id"].astype(int).tolist()

            cur_port_id = 0
            if "port_id" in ship_row and not pd.isna(ship_row["port_id"]):
                try:
                    cur_port_id = int(ship_row["port_id"])
                except Exception:
                    cur_port_id = 0

            try:
                port_index = options_ports.index(cur_port_id)
            except ValueError:
                port_index = 0

            new_port_id = st.selectbox(
                "Перемістити у порт",
                options_ports,
                index=port_index,
                format_func=lambda x: "🌊 У плаванні (без порту)" if x == 0 else port_map.get(x, f"port id={x}")
            )

            # === Статус — випадаючий список ===
            cur_status = str(ship_row.get("status") or "docked")
            try:
                status_index = STATUS_VALUES.index(cur_status)
            except ValueError:
                status_index = 0

            new_status = st.selectbox(
                "Змінити статус",
                STATUS_VALUES,
                index=status_index,
                format_func=status_format,
            )

            # === Компанія-власник ===
            company_ids = [0]
            if not companies_df.empty and "id" in companies_df.columns:
                company_ids += companies_df["id"].astype(int).tolist()

            cur_company_id = 0
            if "company_id" in ship_row and not pd.isna(ship_row["company_id"]):
                try:
                    cur_company_id = int(ship_row["company_id"])
                except Exception:
                    cur_company_id = 0

            try:
                company_index = company_ids.index(cur_company_id)
            except ValueError:
                company_index = 0

            new_company_id = st.selectbox(
                "Компанія-власник (0 = немає)",
                company_ids,
                index=company_index,
                format_func=lambda x: "— (без компанії)" if x == 0 else company_map.get(x, f"company id={x}")
            )

            # === Додаткові поля (опційно) ===
            new_name = st.text_input("Назва корабля", value=ship_row["name"])
            new_type = st.text_input("Тип корабля", value=ship_row["type"])
            new_country = st.text_input("Країна приписки", value=ship_row["country"])

            if st.form_submit_button("Зберегти зміни"):
                if not new_name:
                    st.error("Назва корабля є обов'язковою.")
                else:
                    payload = {
                        "name":       new_name,
                        "type":       new_type,
                        "country":    new_country,
                        "port_id":    int(new_port_id),     # 0 -> У плаванні (NULL в БД)
                        "status":     new_status,           # одне з ['docked', 'loading', 'unloading', 'at_sea']
                        "company_id": int(new_company_id),  # 0 -> без компанії
                    }
                    api.api_put(
                        f"/api/ships/{selected_ship_id}",
                        payload,
                        success_msg=f"Дані корабля '{new_name}' оновлено."
                    )

# ---------- 4. ВИДАЛИТИ КОРАБЕЛЬ ----------
with tab_delete:
    st.subheader("❌ Видалити корабель")

    if ships_df.empty:
        st.info("Немає кораблів для видалення.")
    else:
        ship_ids = ships_df["id"].astype(int).tolist()
        selected_ship_id = st.selectbox(
            "Оберіть корабель для видалення",
            ship_ids,
            format_func=lambda sid: ship_label(sid),
            key="ship_delete_select",
        )

        ship_row = ships_df[ships_df["id"] == selected_ship_id].iloc[0]
        ship_name = ship_row["name"]

        st.warning(
            f"Ви дійсно хочете видалити корабель **{ship_name} (id={selected_ship_id})**?",
            icon="⚠️",
        )

        if st.button("❌ Підтвердити видалення", type="primary"):
            api.api_del(
                f"/api/ships/{selected_ship_id}",
                success_msg=f"Корабель '{ship_name}' видалено."
            )

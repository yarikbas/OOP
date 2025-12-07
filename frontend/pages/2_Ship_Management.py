from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Ships Management", page_icon="🚢", layout="wide")
st.title("🚢 Управління кораблями")

# Flash
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
port_map    = api.get_name_map(ports_df) if not ports_df.empty else {}
company_map = api.get_name_map(companies_df) if not companies_df.empty else {}

# ================== СТАТУСИ КОРАБЛІВ ==================
SHIP_STATUS_OPTIONS = [
    ("docked",    "⚓ docked — у порту"),
    ("loading",   "⬆️ loading — завантажується"),
    ("unloading", "⬇️ unloading — розвантажується"),
    ("departed",  "🚢 departed — відплив"),
]
STATUS_VALUES = [v for v, _ in SHIP_STATUS_OPTIONS]
STATUS_LABELS = {v: label for v, label in SHIP_STATUS_OPTIONS}

def status_format(value: str) -> str:
    return STATUS_LABELS.get(value, value or "невідомо")

def safe_int(x, default=0):
    try:
        if pd.isna(x):
            return default
        return int(x)
    except Exception:
        return default

def ship_label_by_id(sid: int) -> str:
    if ships_df.empty or "id" not in ships_df.columns:
        return f"Ship id={sid}"
    row = ships_df[ships_df["id"] == sid]
    if row.empty:
        return f"Ship id={sid}"
    r = row.iloc[0]
    name = r.get("name", "")
    stype = r.get("type", "")
    return f"{name} (id={sid}, type={stype})"

def port_option_label(pid: int) -> str:
    return port_map.get(pid, f"port id={pid}")

def company_option_label(cid: int) -> str:
    if cid == 0:
        return "— (без компанії)"
    return company_map.get(cid, f"company id={cid}")

# Підготовка списків id
port_ids = []
if not ports_df.empty and "id" in ports_df.columns:
    port_ids = ports_df["id"].dropna().astype(int).tolist()

company_ids = [0]
if not companies_df.empty and "id" in companies_df.columns:
    company_ids += companies_df["id"].dropna().astype(int).tolist()

# Типи кораблів - бажано code
type_codes = []
if not types_df.empty:
    if "code" in types_df.columns:
        type_codes = types_df["code"].dropna().astype(str).tolist()
    elif "name" in types_df.columns:
        type_codes = types_df["name"].dropna().astype(str).tolist()

# ================== STICKY TABS ==================
tab = api.sticky_tabs(
    ["📋 Список кораблів", "➕ Створити корабель", "🛠️ Оновити", "❌ Видалити корабель"],
    "ships_main_tabs",
)

# ---------- 1. СПИСОК ----------
if tab == "📋 Список кораблів":
    st.subheader("📋 Всі кораблі")

    if ships_df.empty:
        st.info("Поки що немає жодного корабля.")
    else:
        f1, f2 = st.columns([2, 1])
        q = f1.text_input("Пошук за назвою", placeholder="введіть частину назви", key="ships_search_q")

        # Порожній дефолт фільтра статусів
        if "ships_status_filter" not in st.session_state:
            st.session_state["ships_status_filter"] = []

        status_filter = f2.multiselect(
            "Фільтр статусів",
            STATUS_VALUES,
            default=st.session_state["ships_status_filter"],
            format_func=status_format,
            key="ships_status_filter",
        )

        view = ships_df.copy()

        if q and "name" in view.columns:
            view = view[view["name"].astype(str).str.contains(q, case=False, na=False)]

        # якщо порожньо — не фільтруємо
        if "status" in view.columns and status_filter:
            view = view[view["status"].isin(status_filter)]

        # Людські назви порту та компанії
        if "port_id" in view.columns:
            view["port"] = view["port_id"].map(
                lambda pid: port_map.get(safe_int(pid), f"port id={pid}")
            )

        if "company_id" in view.columns:
            view["company"] = view["company_id"].map(
                lambda cid: "—" if safe_int(cid) == 0 else company_map.get(safe_int(cid), f"company id={cid}")
            )

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
        st.dataframe(api.df_1based(view), use_container_width=True)

# ---------- 2. CREATE ----------
elif tab == "➕ Створити корабель":
    st.subheader("➕ Створити новий корабель")

    if not port_ids:
        st.warning("Немає портів у БД. Спочатку додайте порти.")
    else:
        with st.form("create_ship_form"):
            name = st.text_input("Назва корабля", placeholder="Mriya Sea")

            if type_codes:
                ship_type = st.selectbox("Тип корабля", type_codes, index=0, key="create_ship_type")
            else:
                ship_type = st.text_input("Тип корабля (текстом)", value="cargo")

            country = st.text_input("Країна приписки", value="Ukraine")

            selected_port_id = st.selectbox(
                "Початкове розташування (порт)",
                port_ids,
                format_func=port_option_label,
                key="create_ship_port",
            )

            selected_status = st.selectbox(
                "Початковий статус",
                STATUS_VALUES,
                format_func=status_format,
                index=0,
                key="create_ship_status",
            )

            selected_company_id = st.selectbox(
                "Компанія-власник",
                company_ids,
                format_func=company_option_label,
                key="create_ship_company",
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
                        "port_id":    int(selected_port_id),
                        "status":     selected_status,
                        "company_id": int(selected_company_id),
                    }
                    api.api_post(
                        "/api/ships",
                        payload,
                        success_msg=f"Корабель '{name}' створено."
                    )

# ---------- 3. UPDATE ----------
elif tab == "🛠️ Оновити":
    st.subheader("🛠️ Оновити дані корабля")

    if ships_df.empty:
        st.info("Немає кораблів для оновлення.")
    else:
        ship_ids = ships_df["id"].dropna().astype(int).tolist()

        selected_ship_id = st.selectbox(
            "Оберіть корабель",
            ship_ids,
            format_func=ship_label_by_id,
            key="ship_update_select",
        )

        ship_row = ships_df[ships_df["id"] == selected_ship_id].iloc[0]

        with st.form("update_ship_form"):
            # Порт
            if not port_ids:
                st.warning("Немає портів у БД. Переміщення неможливе.")
                new_port_id = safe_int(ship_row.get("port_id", 0))
            else:
                cur_port_id = safe_int(ship_row.get("port_id", port_ids[0]))
                if cur_port_id not in port_ids:
                    cur_port_id = port_ids[0]

                port_index = port_ids.index(cur_port_id)

                new_port_id = st.selectbox(
                    "Поточний/новий порт",
                    port_ids,
                    index=port_index,
                    format_func=port_option_label,
                    key="update_ship_port",
                )

            # Статус
            cur_status = str(ship_row.get("status") or "docked")
            status_index = STATUS_VALUES.index(cur_status) if cur_status in STATUS_VALUES else 0

            new_status = st.selectbox(
                "Статус",
                STATUS_VALUES,
                index=status_index,
                format_func=status_format,
                key="update_ship_status",
            )

            # Компанія
            cur_company_id = safe_int(ship_row.get("company_id", 0))
            if cur_company_id not in company_ids:
                cur_company_id = 0
            company_index = company_ids.index(cur_company_id)

            new_company_id = st.selectbox(
                "Компанія-власник",
                company_ids,
                index=company_index,
                format_func=company_option_label,
                key="update_ship_company",
            )

            # Інші поля
            new_name    = st.text_input("Назва корабля", value=str(ship_row.get("name", "")), key="update_ship_name")
            new_type    = st.text_input("Тип корабля", value=str(ship_row.get("type", "")), key="update_ship_type")
            new_country = st.text_input("Країна приписки", value=str(ship_row.get("country", "")), key="update_ship_country")

            if st.form_submit_button("Зберегти зміни"):
                if not new_name:
                    st.error("Назва корабля є обов'язковою.")
                else:
                    payload = {
                        "name":       new_name,
                        "type":       new_type,
                        "country":    new_country,
                        "port_id":    int(new_port_id),
                        "status":     new_status,
                        "company_id": int(new_company_id),
                    }
                    api.api_put(
                        f"/api/ships/{selected_ship_id}",
                        payload,
                        success_msg=f"Дані корабля '{new_name}' оновлено."
                    )

# ---------- 4. DELETE ----------
elif tab == "❌ Видалити корабель":
    st.subheader("❌ Видалити корабель")

    if ships_df.empty:
        st.info("Немає кораблів для видалення.")
    else:
        ship_ids = ships_df["id"].dropna().astype(int).tolist()

        selected_ship_id = st.selectbox(
            "Оберіть корабель для видалення",
            ship_ids,
            format_func=ship_label_by_id,
            key="ship_delete_select",
        )

        ship_row = ships_df[ships_df["id"] == selected_ship_id].iloc[0]
        ship_name = ship_row.get("name", f"id={selected_ship_id}")

        st.warning(
            f"Ви дійсно хочете видалити корабель **{ship_name} (id={selected_ship_id})**?",
            icon="⚠️",
        )

        if st.button("❌ Підтвердити видалення", type="primary", key="ship_delete_btn"):
            api.api_del(
                f"/api/ships/{selected_ship_id}",
                success_msg=f"Корабель '{ship_name}' видалено."
            )

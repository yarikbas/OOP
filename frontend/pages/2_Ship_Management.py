from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Ships Management", page_icon="🚢", layout="wide")
st.title("🚢 Управління кораблями")

# Flash повідомлення
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))


# ================== UI HELPERS ==================
def df_stretch(df: pd.DataFrame):
    try:
        st.dataframe(df, width="stretch")
    except TypeError:
        st.dataframe(df, use_container_width=True)


# ================== ЗАВАНТАЖЕННЯ ДАНИХ ==================
try:
    ships_df      = api.get_ships()
    ports_df      = api.get_ports()
    companies_df  = api.get_companies()
    types_df      = api.get_ship_types()
except Exception as e:
    st.error(f"Не вдалося завантажити дані з backend: {e}")
    st.stop()

# Мапи для відображення
port_map    = api.get_name_map(ports_df) if not ports_df.empty else {}
company_map = api.get_name_map(companies_df) if not companies_df.empty else {}

# === ПІДГОТОВКА ТИПІВ КОРАБЛІВ (Словник Code -> Name) ===
# Ми використовуємо 'code' для запиту на сервер, але 'name' для відображення користувачу
ship_type_map = {}
ship_type_codes = []

if not types_df.empty and "code" in types_df.columns:
    # Заповнюємо мапу
    for _, row in types_df.iterrows():
        c = str(row["code"])
        n = str(row.get("name", c))
        ship_type_map[c] = n
    
    ship_type_codes = list(ship_type_map.keys())

# Функція для відображення в selectbox (показує Назву)
def format_ship_type(code: str) -> str:
    return ship_type_map.get(code, code)


# ================== ПІДГОТОВКА ІНШИХ СПИСКІВ ==================
port_ids = []
if not ports_df.empty and "id" in ports_df.columns:
    port_ids = ports_df["id"].dropna().astype(int).tolist()

def port_label(pid: int) -> str:
    return port_map.get(pid, f"port id={pid}")

company_ids = [0]
if not companies_df.empty and "id" in companies_df.columns:
    company_ids += companies_df["id"].dropna().astype(int).tolist()

def company_label(cid: int) -> str:
    if cid == 0: return "— (без компанії)"
    return company_map.get(cid, f"company id={cid}")

# Статуси
SHIP_STATUS_OPTIONS = [
    ("docked",    "⚓ docked — у порту"),
    ("loading",   "⬆️ loading — завантажується"),
    ("unloading", "⬇️ unloading — розвантажується"),
    ("departed",  "🚢 departed — відплив"),
]
STATUS_VALUES = [v for v, _ in SHIP_STATUS_OPTIONS]
STATUS_LABELS = {v: label for v, label in SHIP_STATUS_OPTIONS}

def status_fmt(val: str) -> str: return STATUS_LABELS.get(val, val)
def safe_int(x): 
    try: return int(x)
    except: return 0

def ship_full_label(sid: int) -> str:
    if ships_df.empty or "id" not in ships_df.columns: return f"#{sid}"
    row = ships_df[ships_df["id"] == sid]
    if row.empty: return f"#{sid}"
    r = row.iloc[0]
    return f"{r.get('name', '')} (#{sid})"


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
        q = f1.text_input("Пошук", placeholder="Назва...", key="sh_q")
        
        if "sh_stat_flt" not in st.session_state: st.session_state["sh_stat_flt"] = []
        stat_flt = f2.multiselect("Статус", STATUS_VALUES, format_func=status_fmt, key="sh_stat_flt")

        view = ships_df.copy()
        if q and "name" in view.columns:
            view = view[view["name"].astype(str).str.contains(q, case=False, na=False)]
        if "status" in view.columns and stat_flt:
            view = view[view["status"].isin(stat_flt)]

        # Human readable
        if "port_id" in view.columns:
            view["port"] = view["port_id"].map(lambda x: port_map.get(safe_int(x), str(x)))
        if "company_id" in view.columns:
            view["company"] = view["company_id"].map(lambda x: "—" if safe_int(x)==0 else company_map.get(safe_int(x), str(x)))
        if "status" in view.columns:
            view["status"] = view["status"].map(status_fmt)
        if "type" in view.columns:
            # Замінюємо код типу на назву
            view["type"] = view["type"].map(lambda x: ship_type_map.get(x, x))

        final_cols = [c for c in ["id", "name", "type", "country", "status", "port", "company"] if c in view.columns]
        df_stretch(api.df_1based(view[final_cols]))

# ---------- 2. CREATE ----------
elif tab == "➕ Створити корабель":
    st.subheader("➕ Створити новий корабель")

    # Жорстка перевірка: якщо немає типів, не даємо створити корабель
    if not ship_type_codes:
        st.error("⛔ У системі немає типів кораблів!")
        st.info("Будь ласка, перейдіть у вкладку **'⚙️ Admin Data' -> 'Управління Типами Кораблів'** та створіть хоча б один тип (наприклад, 'Passenger').")
    elif not port_ids:
        st.warning("⛔ Немає портів. Спочатку додайте порти в 'Admin Data'.")
    else:
        with st.form("create_ship_form"):
            name = st.text_input("Назва корабля", placeholder="Mriya")
            
            # ВИБІР ТИПУ (Користувач бачить Назву, ми беремо Код)
            selected_type_code = st.selectbox(
                "Тип корабля", 
                ship_type_codes, 
                format_func=format_ship_type
            )
            
            country = st.text_input("Країна", value="Ukraine")
            sel_port = st.selectbox("Порт", port_ids, format_func=port_label)
            sel_comp = st.selectbox("Компанія", company_ids, format_func=company_label)

            st.caption("ℹ️ Початковий статус автоматично: **docked**")

            if st.form_submit_button("Створити"):
                if not name:
                    st.error("Введіть назву.")
                else:
                    api.api_post("/api/ships", {
                        "name": name,
                        "type": selected_type_code, # Відправляємо код (напр. 'passenger')
                        "country": country,
                        "port_id": int(sel_port),
                        "status": "docked",
                        "company_id": int(sel_comp)
                    }, success_msg=f"Корабель '{name}' створено.")

# ---------- 3. UPDATE ----------
elif tab == "🛠️ Оновити":
    st.subheader("🛠️ Оновити дані")
    if ships_df.empty:
        st.info("Немає кораблів.")
    else:
        ship_ids = ships_df["id"].dropna().astype(int).tolist()
        sid = st.selectbox("Корабель", ship_ids, format_func=ship_full_label)
        
        row = ships_df[ships_df["id"] == sid].iloc[0]

        with st.form("upd_ship"):
            new_name = st.text_input("Назва", value=str(row.get("name", "")))
            
            # Тип (поточний вибираємо по коду)
            cur_code = str(row.get("type", ""))
            idx_type = 0
            if cur_code in ship_type_codes:
                idx_type = ship_type_codes.index(cur_code)
            
            # Якщо раптом типи зникли, показуємо попередження, але даємо форму
            if ship_type_codes:
                new_type = st.selectbox("Тип", ship_type_codes, index=idx_type, format_func=format_ship_type)
            else:
                st.warning("Типи кораблів відсутні в довіднику.")
                new_type = st.text_input("Тип (код)", value=cur_code, disabled=True)

            new_country = st.text_input("Країна", value=str(row.get("country", "")))
            
            cur_pid = safe_int(row.get("port_id", 0))
            pidx = port_ids.index(cur_pid) if cur_pid in port_ids else 0
            new_port = st.selectbox("Порт", port_ids, index=pidx, format_func=port_label)

            cur_stat = str(row.get("status", "docked"))
            sidx = STATUS_VALUES.index(cur_stat) if cur_stat in STATUS_VALUES else 0
            new_stat = st.selectbox("Статус", STATUS_VALUES, index=sidx, format_func=status_fmt)

            cur_cid = safe_int(row.get("company_id", 0))
            cidx = company_ids.index(cur_cid) if cur_cid in company_ids else 0
            new_comp = st.selectbox("Компанія", company_ids, index=cidx, format_func=company_label)

            if st.form_submit_button("Зберегти"):
                api.api_put(f"/api/ships/{sid}", {
                    "name": new_name,
                    "type": new_type,
                    "country": new_country,
                    "port_id": int(new_port),
                    "status": new_stat,
                    "company_id": int(new_comp)
                }, success_msg="Оновлено.")

# ---------- 4. DELETE ----------
elif tab == "❌ Видалити корабель":
    st.subheader("❌ Видалити корабель")
    if ships_df.empty:
        st.info("Немає кораблів.")
    else:
        ship_ids = ships_df["id"].dropna().astype(int).tolist()
        del_id = st.selectbox("Оберіть корабель", ship_ids, format_func=ship_full_label, key="del_sel")
        del_name = ships_df[ships_df["id"] == del_id].iloc[0].get("name", "")

        if st.button(f"❌ Видалити '{del_name}'", type="primary"):
            api.api_del(f"/api/ships/{del_id}", success_msg="Видалено.")
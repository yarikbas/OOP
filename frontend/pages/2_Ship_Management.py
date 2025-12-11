from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Ships Management", page_icon="🚢", layout="wide")
api.inject_theme()

# Sidebar identity and health
st.sidebar.title("🚢 Fleet Manager")
st.sidebar.caption("Ships")
from common import get_health
_h = get_health()

# Center title
col_l, col_c, col_r = st.columns([1, 3, 1])
with col_c:
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

# Агрегати для зручних KPI
in_port_df = ships_df[ships_df["status"] != "departed"] if "status" in ships_df.columns else ships_df.copy()
try:
    with_company = int((ships_df["company_id"].fillna(0) != 0).sum()) if "company_id" in ships_df.columns else 0
except Exception:
    with_company = 0
ports_in_use = 0
popular_port = "—"
if not ships_df.empty and "port_id" in ships_df.columns:
    ports_in_use = ships_df["port_id"].fillna(0).astype(int).replace(0, pd.NA).dropna().nunique()
    try:
        top_port_id = ships_df["port_id"].value_counts().idxmax()
        popular_port = port_map.get(int(top_port_id), str(top_port_id))
    except Exception:
        popular_port = "—"

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
    ["📋 Список кораблів", "➕ Створити корабель", "🛠️ Оновити", "🚢 Відправити корабель", "❌ Видалити корабель"],
    "ships_main_tabs",
)

# ---------- 1. СПИСОК ----------
if tab == "📋 Список кораблів":
    st.subheader("📋 Всі кораблі")

    if ships_df.empty:
        st.info("Поки що немає жодного корабля.")
    else:
        snap = st.container()
        with snap:
            s1, s2, s3, s4, s5 = st.columns([1,1,1,1.5,1.2])
            s1.metric("Флот всього", len(ships_df))
            s2.metric("У портах", len(in_port_df))
            s3.metric("З компанією", with_company)
            s4.metric("Порт-лидер", popular_port)
            
            # Кнопка обробки прибуттів
            departed_count = (ships_df["status"] == "departed").sum() if "status" in ships_df.columns else 0
            s5.markdown(f"<div style='padding-top: 8px'><b>У рейсі:</b> {departed_count}</div>", unsafe_allow_html=True)
            if s5.button("🕐 Обробити прибуття", help="Перевірити які кораблі вже прибули за розкладом"):
                with st.spinner("Перевіряємо прибуття..."):
                    try:
                        result = api.call("POST", "/api/ships/process-arrivals")
                        processed = result.get("processed", 0)
                        if processed > 0:
                            st.success(f"✅ Прибуло кораблів: {processed}")
                            st.rerun()
                        else:
                            st.info("Немає кораблів що прибули")
                    except Exception as e:
                        st.error(f"Помилка: {e}")

        with st.expander("Фільтри та пошук", expanded=True):
            c1, c2, c3, c4 = st.columns([1.6, 1, 1, 1])
            q = c1.text_input("Пошук за назвою або країною", placeholder="Mriya / Greece", key="sh_q")

            if "sh_stat_flt" not in st.session_state:
                st.session_state["sh_stat_flt"] = []
            stat_flt = c2.multiselect("Статус", STATUS_VALUES, format_func=status_fmt, key="sh_stat_flt")

            if "sh_type_flt" not in st.session_state:
                st.session_state["sh_type_flt"] = []
            type_flt = c3.multiselect("Тип корабля", ship_type_codes, format_func=format_ship_type, key="sh_type_flt")

            if "sh_company_flt" not in st.session_state:
                st.session_state["sh_company_flt"] = []
            company_flt = c4.multiselect("Компанії", company_ids[1:] if len(company_ids) > 1 else [], format_func=company_label, key="sh_company_flt")

            c5, c6 = st.columns([1, 1])
            if "sh_port_flt" not in st.session_state:
                st.session_state["sh_port_flt"] = []
            port_flt = c5.multiselect("Порти", port_ids, format_func=port_label, key="sh_port_flt")
            only_company = c6.checkbox("Тільки з компанією", value=False, key="sh_only_company")

            if st.button("Очистити фільтри", key="sh_clear_filters"):
                for k in ["sh_q", "sh_stat_flt", "sh_type_flt", "sh_company_flt", "sh_port_flt", "sh_only_company"]:
                    st.session_state.pop(k, None)
                st.experimental_rerun()

        view = ships_df.copy()

        if q and "name" in view.columns:
            mask_name = view["name"].astype(str).str.contains(q, case=False, na=False)
            mask_country = view["country"].astype(str).str.contains(q, case=False, na=False) if "country" in view.columns else False
            view = view[mask_name | mask_country]
        if "status" in view.columns and stat_flt:
            view = view[view["status"].isin(stat_flt)]
        if "type" in view.columns and type_flt:
            view = view[view["type"].isin(type_flt)]
        if "company_id" in view.columns and company_flt:
            view = view[view["company_id"].isin(company_flt)]
        if "port_id" in view.columns and port_flt:
            view = view[view["port_id"].isin(port_flt)]
        if only_company and "company_id" in view.columns:
            view = view[view["company_id"].fillna(0).astype(int) != 0]

        # Human readable
        if "port_id" in view.columns:
            view["port"] = view["port_id"].map(lambda x: port_map.get(safe_int(x), str(x)))
        if "company_id" in view.columns:
            view["company"] = view["company_id"].map(lambda x: "—" if safe_int(x)==0 else company_map.get(safe_int(x), str(x)))
        if "status" in view.columns:
            view["status"] = view["status"].map(status_fmt)
        if "type" in view.columns:
            view["type"] = view["type"].map(lambda x: ship_type_map.get(x, x))

        final_cols = [c for c in ["id", "name", "type", "country", "status", "port", "company", "speed_knots"] if c in view.columns]

        # Summary chips
        met1, met2, met3 = st.columns(3)
        met1.metric("Кількість кораблів", len(view))
        if "company_id" in view.columns:
            met2.metric("З компанією", int((view["company_id"].fillna(0) != 0).sum()))
        if "status" in view.columns:
            top_status = view["status"].mode()[0] if not view.empty else "—"
            met3.metric("Найчастіший статус", str(top_status))

        df_stretch(api.df_1based(view[final_cols]))

        # --- Voyage tracking section for departed ships ---
        departed_ships = ships_df[ships_df.get("status", "") == "departed"].copy() if "status" in ships_df.columns else pd.DataFrame()
        
        if not departed_ships.empty:
            st.markdown("---")
            st.subheader("⛵ Рейси у путі (Voyage Tracking)")
            
            # Prepare voyage data
            from datetime import datetime
            
            voyage_data = []
            for idx, row in departed_ships.iterrows():
                ship_id = safe_int(row.get("id", 0))
                ship_name = row.get("name", "Unknown")
                departed_at = row.get("departed_at", "")
                eta = row.get("eta", "")
                distance = row.get("voyage_distance_km", 0)
                dest_port_id = safe_int(row.get("destination_port_id", 0))
                dest_port = port_map.get(dest_port_id, "Unknown")
                
                # Parse timestamps
                try:
                    if departed_at:
                        depart_dt = datetime.fromisoformat(departed_at.replace('Z', '+00:00'))
                        depart_str = depart_dt.strftime("%Y-%m-%d %H:%M")
                    else:
                        depart_str = "—"
                    
                    if eta:
                        eta_dt = datetime.fromisoformat(eta.replace('Z', '+00:00'))
                        eta_str = eta_dt.strftime("%Y-%m-%d %H:%M")
                        
                        # Calculate progress
                        now = datetime.now(eta_dt.tzinfo) if eta_dt.tzinfo else datetime.now()
                        total_duration = eta_dt.timestamp() - (depart_dt.timestamp() if departed_at else now.timestamp())
                        elapsed_duration = now.timestamp() - (depart_dt.timestamp() if departed_at else now.timestamp())
                        
                        progress = min(100, max(0, (elapsed_duration / total_duration * 100) if total_duration > 0 else 0))
                    else:
                        eta_str = "—"
                        progress = 0
                    
                    distance_str = f"{distance:.0f} км" if distance > 0 else "—"
                    
                    voyage_data.append({
                        "Ship": ship_name,
                        "Destination": dest_port,
                        "Distance": distance_str,
                        "Departed": depart_str,
                        "ETA": eta_str,
                        "Progress %": progress
                    })
                except Exception as e:
                    st.warning(f"Error parsing voyage data for {ship_name}: {e}")
            
            if voyage_data:
                voyage_df = pd.DataFrame(voyage_data)
                
                # Show as expandable cards
                for idx, voyage in enumerate(voyage_data):
                    with st.expander(f"🚢 {voyage['Ship']} → {voyage['Destination']}", expanded=(idx == 0)):
                        col1, col2, col3, col4 = st.columns(4)
                        col1.metric("Відстань", voyage['Distance'])
                        col2.metric("Відправка", voyage['Departed'])
                        col3.metric("ETA", voyage['ETA'])
                        col4.metric("Прогрес", f"{voyage['Progress %']:.1f}%")
                        
                        # Progress bar
                        st.progress(min(100, voyage['Progress %']) / 100.0)

        # --- Dashboard: fleet story ---
        st.markdown("---")
        st.subheader("Флот: зріз та розподіли")

        col_mix, col_status = st.columns(2)

        with col_mix:
            if not ships_df.empty and "type" in ships_df.columns:
                try:
                    by_type = ships_df["type"].fillna("(unknown)").map(lambda x: ship_type_map.get(x, x))
                    counts = by_type.value_counts().rename_axis("type").reset_index(name="count").set_index("type")
                    st.markdown("**Структура за типами**")
                    st.bar_chart(counts)
                except Exception:
                    st.caption("Немає даних по типах.")

            if not ships_df.empty and "company_id" in ships_df.columns:
                try:
                    comp_map = api.get_name_map(companies_df, id_col="id", name_col="name")
                    comp_series = ships_df["company_id"].fillna(0).astype(int).map(lambda x: comp_map.get(x, "— (no company)"))
                    comp_counts = comp_series.value_counts().rename_axis("company").reset_index(name="count").set_index("company")
                    st.markdown("**Топ компаній**")
                    st.bar_chart(comp_counts)
                except Exception:
                    st.caption("Немає даних по компаніях.")

        with col_status:
            if not ships_df.empty and "status" in ships_df.columns:
                try:
                    status_counts = ships_df["status"].fillna("(unknown)").value_counts().rename_axis("status").reset_index(name="count").set_index("status")
                    st.markdown("**Стан флоту**")
                    st.bar_chart(status_counts)
                except Exception:
                    st.caption("Немає даних по статусах.")

            if not ships_df.empty and "port_id" in ships_df.columns:
                try:
                    port_counts = ships_df["port_id"].fillna(0).astype(int)
                    port_counts = port_counts.map(lambda x: port_map.get(x, "—")).value_counts().rename_axis("port").reset_index(name="count").set_index("port")
                    st.markdown("**Навантаження портів**")
                    st.bar_chart(port_counts)
                except Exception:
                    st.caption("Немає даних по портах.")

        # Downloads: raw ships CSV and aggregated summaries
        with st.expander("Download data"):
            try:
                csv_ships = ships_df.to_csv(index=False)
                st.download_button("Download ships CSV", data=csv_ships, file_name="ships.csv", mime="text/csv")

                agg_parts = []
                try:
                    if 'counts' in locals():
                        agg_parts.append(counts.reset_index().assign(metric="by_type"))
                    if 'status_counts' in locals():
                        agg_parts.append(status_counts.reset_index().assign(metric="by_status"))
                    if 'comp_counts' in locals():
                        agg_parts.append(comp_counts.reset_index().assign(metric="by_company"))
                    if agg_parts:
                        agg = pd.concat(agg_parts, ignore_index=True)
                    else:
                        agg = None
                except Exception:
                    agg = None

                if agg is not None:
                    st.download_button("Download aggregates CSV", data=agg.to_csv(index=False), file_name="ships_aggregates.csv", mime="text/csv")
            except Exception as e:
                st.write("Download unavailable: ", e)

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
            
            # Швидкість корабля
            speed_knots = st.number_input(
                "Швидкість (вузли)", 
                min_value=5, 
                max_value=50, 
                value=20, 
                help="Типова швидкість для різних типів суден: 15-20 вузлів"
            )

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
                        "company_id": int(sel_comp),
                        "speed_knots": speed_knots
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
            
            # Швидкість корабля
            cur_speed = float(row.get("speed_knots", 20.0))
            new_speed = st.number_input(
                "Швидкість (вузли)", 
                min_value=5, 
                max_value=50, 
                value=int(cur_speed), 
                help="Типова швидкість для різних типів суден: 15-20 вузлів"
            )

            if st.form_submit_button("Зберегти"):
                api.api_put(f"/api/ships/{sid}", {
                    "name": new_name,
                    "type": new_type,
                    "country": new_country,
                    "port_id": int(new_port),
                    "status": new_stat,
                    "company_id": int(new_comp),
                    "speed_knots": new_speed
                }, success_msg="Оновлено.")

# ---------- 4. ВІДПРАВИТИ КОРАБЕЛЬ ----------
elif tab == "🚢 Відправити корабель":
    st.subheader("🚢 Відправити корабель у рейс")
    
    st.markdown("""
    **Бізнес-правило:** Корабель може відплисти (`departed`) тільки якщо:
    - На ньому є активний **капітан** в екіпажі
    - Ти вказуєш порт призначення і час відправки
    
    Система автоматично розрахує ETA на основі відстані і типової швидкості.
    """)

    if ships_df.empty:
        st.info("Немає кораблів.")
    elif not port_ids:
        st.warning("Немає портів для призначення.")
    else:
        # Фільтруємо тільки кораблі не у статусі departed
        available_ships = ships_df.copy()
        if "status" in available_ships.columns:
            available_ships = available_ships[available_ships["status"] != "departed"]
        
        if available_ships.empty:
            st.info("Всі кораблі вже у рейсі (departed).")
        else:
            available_ids = available_ships["id"].dropna().astype(int).tolist()
            
            selected_ship_id = st.selectbox(
                "Оберіть корабель для відправки",
                available_ids,
                format_func=ship_full_label,
                key="depart_ship_select",
            )
            
            ship_row = ships_df[ships_df["id"] == selected_ship_id].iloc[0]
            ship_name = ship_row.get("name", "")
            current_port_id = safe_int(ship_row.get("port_id", 0))
            current_port = port_map.get(current_port_id, "невідомо")
            
            st.markdown(f"**Обраний корабель:** {ship_name}")
            st.markdown(f"**Поточний порт:** {current_port}")
            
            # Перевіряємо екіпаж
            try:
                crew_df = api.get_ship_crew(selected_ship_id)
                people_df = api.get_people()
                
                has_captain = False
                if not crew_df.empty and not people_df.empty:
                    if "person_id" in crew_df.columns and "id" in people_df.columns:
                        crew_ids = crew_df["person_id"].dropna().astype(int).tolist()
                        crew_people = people_df[people_df["id"].isin(crew_ids)]
                        
                        if "rank" in crew_people.columns:
                            # Шукаємо капітана (українською або англійською)
                            ranks = crew_people["rank"].astype(str).str.lower()
                            has_captain = any(r in ["капітан", "captain"] for r in ranks)
                
                if has_captain:
                    st.success("✅ На кораблі є капітан — можна відправляти!")
                else:
                    st.error("❌ На кораблі немає активного капітана. Спочатку призначте капітана в екіпаж.")
                    st.info("Перейдіть на вкладку **'Crew & People'** → **'Управління Екіпажами'** та призначте людину з рангом 'Капітан'.")
                    st.stop()
            except Exception as e:
                st.warning(f"Не вдалося перевірити екіпаж: {e}")
            
            # Вибір порту призначення
            dest_port = st.selectbox(
                "Порт призначення",
                port_ids,
                format_func=port_label,
                key="depart_dest_port",
            )
            
            # Час-базована механіка путешествия
            st.subheader("⏱️ Параметри рейсу")
            
            col1, col2 = st.columns(2)
            with col1:
                from datetime import datetime, timedelta
                
                # Режим введення часу
                manual_time = st.checkbox("✍️ Ввести час вручну", value=False, key="manual_time_input")
                
                if manual_time:
                    # Ручний ввід у форматі ISO або будь-якому зручному
                    datetime_str = st.text_input(
                        "Дата і час відправки",
                        value=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                        help="Формат: YYYY-MM-DD HH:MM:SS (наприклад: 2025-12-11 20:30:00)",
                        key="manual_datetime"
                    )
                    try:
                        from datetime import datetime as dt
                        departed_at = dt.strptime(datetime_str, "%Y-%m-%d %H:%M:%S").isoformat()
                        st.success(f"✅ Відправка: {datetime_str}")
                    except ValueError:
                        st.error("❌ Неправильний формат дати/часу. Використайте: YYYY-MM-DD HH:MM:SS")
                        st.stop()
                else:
                    # Стандартний вибір через віджети
                    depart_time = st.time_input("Час відправки", value=datetime.now().time(), key="depart_time")
                    depart_date = st.date_input("Дата відправки", value=datetime.now().date(), key="depart_date")
                    
                    # Комбінуємо дату та час у ISO формат
                    from datetime import datetime as dt
                    departed_at = dt.combine(depart_date, depart_time).isoformat()
            
            with col2:
                # Отримуємо координати поточного порту і порту призначення
                current_port_data = ports_df[ports_df["id"] == current_port_id]
                dest_port_data = ports_df[ports_df["id"] == dest_port]
                
                # Автоматично розраховуємо відстань за координатами
                voyage_distance = 0
                if not current_port_data.empty and not dest_port_data.empty:
                    try:
                        # Використовуємо правильні назви полів з API: lat і lon
                        lat1 = float(current_port_data.iloc[0].get("lat", 0))
                        lon1 = float(current_port_data.iloc[0].get("lon", 0))
                        lat2 = float(dest_port_data.iloc[0].get("lat", 0))
                        lon2 = float(dest_port_data.iloc[0].get("lon", 0))
                        
                        # Використовуємо формулу гаверсинуса для розрахунку відстані
                        voyage_distance = api.haversine_distance(lat1, lon1, lat2, lon2)
                        st.info(f"📏 Відстань від {current_port} до {port_map.get(dest_port, 'порту')}: **{voyage_distance:.1f} км**")
                    except Exception as e:
                        st.warning(f"Не вдалося розрахувати відстань: {e}")
                        voyage_distance = 500  # Значення за замовчуванням
                
                # Отримуємо швидкість корабля з бази даних
                ship_speed_knots = float(ship_row.get("speed_knots", 20.0))
                
                st.info(f"⚓ Швидкість судна: **{ship_speed_knots:.1f} вузлів** ({ship_speed_knots * 1.852:.1f} км/год)")
                
                # Розраховуємо час в дорозі
                speed_kmh = ship_speed_knots * 1.852  # конвертація вузлів в км/год
                voyage_hours = voyage_distance / speed_kmh if speed_kmh > 0 else 0
                voyage_days = voyage_hours / 24
                
                st.info(f"📊 Час у дорозі: **{voyage_hours:.1f} годин** ({voyage_days:.2f} днів)")

            
            # Розраховуємо ETA
            from datetime import datetime as dt
            depart_dt = dt.fromisoformat(departed_at)
            eta_dt = depart_dt + timedelta(hours=voyage_hours)
            eta_str = eta_dt.isoformat()
            
            st.info(f"✈️ Очікуване прибуття: **{eta_dt.strftime('%Y-%m-%d %H:%M')}**")
            
            st.warning("⚠️ Після відправки статус зміниться на **departed** і корабель покине поточний порт.", icon="⚠️")
            
            if st.button(f"🚢 Відправити '{ship_name}' до {port_map.get(dest_port, 'порту')}", type="primary"):
                api.api_put(f"/api/ships/{selected_ship_id}", {
                    "status": "departed",
                    "port_id": int(dest_port),
                    "destination_port_id": int(dest_port),
                    "departed_at": departed_at,
                    "eta": eta_str,
                    "voyage_distance_km": voyage_distance,
                    "speed_knots": ship_speed_knots
                }, success_msg=f"Корабель '{ship_name}' відправлено у рейс! ETA: {eta_dt.strftime('%Y-%m-%d %H:%M')}")


# ---------- 5. DELETE ----------
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
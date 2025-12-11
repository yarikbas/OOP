from __future__ import annotations

import re
import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Admin / Data", page_icon="⚙️", layout="wide")
api.inject_theme()

# Sidebar identity and health
st.sidebar.title("🚢 Fleet Manager")
st.sidebar.caption("Admin / Data")
from common import get_health
_h = get_health()


# Center title
col_l, col_c, col_r = st.columns([1, 3, 1])
with col_c:
    st.title("⚙️ Адмін-Панель та Дані")
st.caption("Тут керуємо Портами та МОДЕЛЯМИ кораблів.")

# Flash
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))


# ================== UI HELPERS ==================
def df_stretch(df: pd.DataFrame, **kwargs):
    try:
        st.dataframe(df, width="stretch", **kwargs)
    except TypeError:
        st.dataframe(df, use_container_width=True, **kwargs)


# ================== BASE SHIP TYPES ==================
# Це жорстко зашиті категорії, які розуміє C++ бекенд
BASE_TYPES = [
    ("cargo",     "Вантажний"),
    ("military",  "Військовий"),
    ("research",  "Дослідницький"),
    ("passenger", "Пасажирський"),
]
BASE_LABEL = {c: n for c, n in BASE_TYPES}
BASE_CODES = [c for c, _ in BASE_TYPES]


def split_model_code(full_code: str) -> tuple[str, str]:
    """Розбиває code='cargo_panamax' на ('cargo', 'panamax')"""
    if not full_code: return "", ""
    if "_" not in full_code: return "", full_code
    base, rest = full_code.split("_", 1)
    return base, rest


def generate_slug(text: str) -> str:
    """
    Генерує чистий хвостик коду з назви:
    "Super Tanker 3000" -> "super-tanker-3000"
    """
    s = str(text).lower().strip()
    # Замінюємо пробіли на дефіси
    s = re.sub(r'\s+', '-', s)
    # Залишаємо тільки латиницю, цифри і дефіс
    # (Кирилицю можна було б транслітерувати, але для простоти просто чистимо)
    s = re.sub(r'[^a-z0-9\-]', '', s)
    return s


# ================== LOAD ==================
try:
    ports_df = api.get_ports()
    types_df = api.get_ship_types()
    port_map = api.get_name_map(ports_df)
except Exception as e:
    st.error(f"Не вдалося завантажити довідники: {e}")
    st.stop()


# ================== MAIN TABS ==================
tab = api.sticky_tabs(
    ["⚓ Управління Портами", "📋 Моделі Кораблів", "📥 Імпорт реальних даних"],
    "admin_main_tabs",
)

# -------------------------------------------------------------------
#                               PORTS
# -------------------------------------------------------------------
if tab == "⚓ Управління Портами":
    st.subheader("Управління Портами")

    crud = api.sticky_tabs(
        ["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"],
        "admin_ports_crud_tabs",
    )

    # Список
    if crud == "📋 Список":
        with st.expander("Фільтри портів", expanded=True):
            f1, f2, f3 = st.columns([2, 1, 1])
            port_search = f1.text_input("Пошук за назвою/регіоном", key="port_filter_search")
            regions = sorted([r for r in ports_df.get("region", pd.Series(dtype=str)).dropna().unique()]) if not ports_df.empty else []
            region_sel = f2.selectbox("Регіон", options=["(усі)"] + regions, index=0, key="port_filter_region")
            sort_sel = f3.selectbox("Сортування", ["ID ↑", "Назва ↑", "Назва ↓"], key="port_filter_sort")
            if st.button("Очистити", key="port_filter_reset"):
                st.session_state["port_filter_search"] = ""
                st.session_state["port_filter_region"] = "(усі)"
                st.session_state["port_filter_sort"] = "ID ↑"
                st.rerun()

        filtered_ports = ports_df.copy()
        if port_search:
            mask_name = filtered_ports.get("name", pd.Series(dtype=str)).astype(str).str.contains(port_search, case=False, na=False)
            mask_region = filtered_ports.get("region", pd.Series(dtype=str)).astype(str).str.contains(port_search, case=False, na=False)
            filtered_ports = filtered_ports[mask_name | mask_region]

        if region_sel != "(усі)" and "region" in filtered_ports.columns:
            filtered_ports = filtered_ports[filtered_ports["region"] == region_sel]

        if not filtered_ports.empty:
            if sort_sel == "Назва ↑" and "name" in filtered_ports.columns:
                filtered_ports = filtered_ports.sort_values(by="name", ascending=True)
            elif sort_sel == "Назва ↓" and "name" in filtered_ports.columns:
                filtered_ports = filtered_ports.sort_values(by="name", ascending=False)
            else:
                filtered_ports = filtered_ports.sort_values(by="id", ascending=True, na_position="last")

        if filtered_ports.empty:
            st.info("Портів ще немає.")
        else:
            df_stretch(api.df_1based(filtered_ports))

    # Створити
    elif crud == "➕ Створити":
        with st.form("create_port_form"):
            name = st.text_input("Назва порту", placeholder="Odesa", key="create_port_name")
            region = st.text_input("Регіон", placeholder="Europe", key="create_port_region")
            lat = st.number_input("Широта", value=46.48, format="%.6f", key="create_port_lat")
            lon = st.number_input("Довгота", value=30.72, format="%.6f", key="create_port_lon")

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
        if ports_df.empty:
            st.info("Немає портів для оновлення.")
        else:
            port_ids = ports_df["id"].tolist()
            pid = st.selectbox("Оберіть порт", port_ids, format_func=lambda x: port_map.get(x, "N/A"))
            row = ports_df[ports_df["id"] == pid].iloc[0]

            with st.form("update_port_form"):
                new_name = st.text_input("Назва", value=str(row.get('name', "")))
                new_region = st.text_input("Регіон", value=str(row.get('region', "")))
                new_lat = st.number_input("Широта", value=float(row.get('lat', 0.0)), format="%.6f")
                new_lon = st.number_input("Довгота", value=float(row.get('lon', 0.0)), format="%.6f")

                if st.form_submit_button("Оновити порт"):
                    api.api_put(
                        f"/api/ports/{pid}",
                        {"name": new_name, "region": new_region, "lat": new_lat, "lon": new_lon},
                        success_msg=f"Порт '{new_name}' оновлено."
                    )

    # Видалити
    elif crud == "❌ Видалити":
        if ports_df.empty:
            st.info("Немає портів для видалення.")
        else:
            pid = st.selectbox("Порт для видалення", ports_df["id"].tolist(), format_func=lambda x: port_map.get(x, "N/A"))
            pname = port_map.get(pid, "N/A")

            st.warning("Видалення порту призведе до помилки, якщо там є кораблі!", icon="⚠️")
            if st.button(f"❌ Видалити '{pname}'", type="primary"):
                api.api_del(f"/api/ports/{pid}", success_msg=f"Порт '{pname}' видалено.")


# -------------------------------------------------------------------
#                           SHIP MODELS
# -------------------------------------------------------------------
elif tab == "📋 Моделі Кораблів":
    st.subheader("Моделі кораблів")
    st.caption("Створюйте моделі (наприклад 'Panamax', 'Cruiser') на основі 4-х базових категорій.")

    crud = api.sticky_tabs(
        ["📋 Список моделей", "➕ Створити модель", "🛠️ Оновити модель", "❌ Видалити модель"],
        "admin_models_crud_tabs",
    )

    # --------- LIST ---------
    if crud == "📋 Список моделей":
        with st.expander("Фільтри моделей", expanded=True):
            f1, f2 = st.columns([2, 1])
            model_search = f1.text_input("Пошук за назвою/кодом", key="model_filter_search")
            base_opts = ["(усі)"] + BASE_CODES
            base_sel = f2.selectbox("Базова категорія", base_opts, index=0, key="model_filter_base")
            if st.button("Очистити", key="model_filter_reset"):
                st.session_state["model_filter_search"] = ""
                st.session_state["model_filter_base"] = "(усі)"
                st.rerun()

        filtered_types = types_df.copy()
        if model_search:
            mask_name = filtered_types.get("name", pd.Series(dtype=str)).astype(str).str.contains(model_search, case=False, na=False)
            mask_code = filtered_types.get("code", pd.Series(dtype=str)).astype(str).str.contains(model_search, case=False, na=False)
            filtered_types = filtered_types[mask_name | mask_code]

        if base_sel != "(усі)" and "code" in filtered_types.columns:
            filtered_types = filtered_types[filtered_types["code"].astype(str).str.startswith(f"{base_sel}_")]

        if filtered_types.empty:
            st.info("Моделей ще немає.")
        else:
            view = filtered_types.copy()
            if "code" in view.columns:
                bases, models, labels = [], [], []
                for v in view["code"].astype(str).tolist():
                    b, m = split_model_code(v)
                    bases.append(b)
                    models.append(m)
                    labels.append(BASE_LABEL.get(b, b))
                view["base_type"] = labels
                view["technical_suffix"] = models

            cols = ["id", "base_type", "name", "technical_suffix", "description"]
            final_cols = [c for c in cols if c in view.columns]
            df_stretch(api.df_1based(view[final_cols]))

    # --------- CREATE MODEL ---------
    elif crud == "➕ Створити модель":
        with st.form("create_model_form"):
            # 1. Вибір категорії (це впливає на бізнес-логіку)
            base_code = st.selectbox(
                "Категорія корабля (впливає на вимоги до екіпажу)",
                options=BASE_CODES,
                format_func=lambda c: BASE_LABEL.get(c, c),
                help="Вантажний потребує інженера, Військовий - солдата тощо.",
            )

            # 2. Назва моделі
            model_name = st.text_input(
                "Назва моделі",
                placeholder="Super Tanker 3000",
                help="Введіть зрозумілу назву.",
            )

            # 3. Автогенерація коду (візуалізація)
            auto_code = ""
            if model_name:
                slug = generate_slug(model_name)
                auto_code = f"{base_code}_{slug}"
                st.caption(f"🔒 Технічний код буде згенеровано автоматично: **`{auto_code}`**")
            else:
                st.caption("🔒 Технічний код буде згенеровано після введення назви.")

            description = st.text_area("Опис (опційно)", placeholder="Опис характеристик...")

            if st.form_submit_button("Створити модель"):
                if not model_name.strip():
                    st.error("Введіть назву моделі.")
                elif not generate_slug(model_name):
                    st.error("Назва повинна містити хоча б одну латинську літеру або цифру.")
                else:
                    api.api_post(
                        "/api/ship-types",
                        {
                            "code": auto_code,
                            "name": model_name.strip(),
                            "description": description,
                        },
                        success_msg=f"Модель '{model_name}' створено (код: {auto_code}).",
                    )

    # --------- UPDATE MODEL ---------
    elif crud == "🛠️ Оновити модель":
        if types_df.empty:
            st.info("Немає моделей.")
        else:
            def model_label(tid):
                r = types_df[types_df["id"] == tid].iloc[0]
                return f"{r.get('name')} (id={tid})"

            tid = st.selectbox("Оберіть модель", types_df["id"].tolist(), format_func=model_label)
            row = types_df[types_df["id"] == tid].iloc[0]

            with st.form("upd_mod"):
                st.info(f"Редагування моделі: **{row.get('name')}**")
                # Код міняти не даємо, бо це зламає існуючі кораблі
                st.text_input("Технічний код (незмінний)", value=str(row.get('code')), disabled=True)
                
                new_name = st.text_input("Назва моделі", value=str(row.get('name', '')))
                new_desc = st.text_area("Опис", value=str(row.get('description', '')))

                if st.form_submit_button("Зберегти зміни"):
                    if new_name.strip():
                        api.api_put(
                            f"/api/ship-types/{tid}",
                            {
                                "code": str(row.get('code')), # старий код
                                "name": new_name.strip(),
                                "description": new_desc
                            },
                            success_msg="Модель оновлено."
                        )
                    else:
                        st.error("Назва не може бути порожньою.")

    # --------- DELETE MODEL ---------
    elif crud == "❌ Видалити модель":
        if types_df.empty:
            st.info("Немає моделей.")
        else:
            def model_label2(tid):
                r = types_df[types_df["id"] == tid].iloc[0]
                return f"{r.get('name')} (id={tid})"

            tid = st.selectbox("Модель для видалення", types_df["id"].tolist(), format_func=model_label2, key="del_mod")
            row = types_df[types_df["id"] == tid].iloc[0]
            name = str(row.get("name"))

            st.warning("Видалення моделі зламає кораблі, які її використовують!", icon="⚠️")
            
            if st.button(f"❌ Видалити '{name}'", type="primary"):
                api.api_del(f"/api/ship-types/{tid}", success_msg=f"Модель '{name}' видалено.")# Append this to the end of 5_Admin_Data.py

# -------------------------------------------------------------------
#                     ІМПОРТ РЕАЛЬНИХ ДАНИХ
# -------------------------------------------------------------------
elif tab == "📥 Імпорт реальних даних":
    st.subheader("📥 Імпорт реальних даних про кораблі та порти")
    
    st.markdown("""
    **Доступні джерела:**
    - 🚢 **Кораблі:** Dataset з Kaggle/GitHub
    - ⚓ **Порти:** OpenStreetMap Nominatim (безкоштовно)
    - 🌍 **Координати:** автоматичне геокодування
    """)

    import_tab = api.sticky_tabs(
        ["🚢 Імпорт кораблів (CSV)", "⚓ Імпорт портів (CSV)", "🌍 Геокодування портів"],
        "import_data_tabs",
    )

    # --------- ІМПОРТ КОРАБЛІВ ---------
    if import_tab == "🚢 Імпорт кораблів (CSV)":
        st.markdown("### Завантажити кораблі з CSV файлу")
        
        st.markdown("""
        **Формат CSV:** `name,type,country,port_name,company_name`
        
        **Приклад:**
        ```
        Ever Given,cargo,Egypt,Port Said,Evergreen Marine
        Titanic II,passenger,USA,Miami,White Star Line
        USS Gerald Ford,military,USA,Norfolk,US Navy
        ```
        """)
        
        uploaded_ships = st.file_uploader(
            "Виберіть CSV файл з кораблями",
            type=["csv"],
            key="upload_ships",
        )
        
        if uploaded_ships:
            try:
                ships_import_df = pd.read_csv(uploaded_ships)
                
                st.markdown("**Попередній перегляд:**")
                st.dataframe(ships_import_df.head(10), use_container_width=True)
                
                required_cols = ["name", "type", "country"]
                missing = [c for c in required_cols if c not in ships_import_df.columns]
                
                if missing:
                    st.error(f"❌ Відсутні обов'язкові колонки: {', '.join(missing)}")
                else:
                    st.success(f"✅ Знайдено {len(ships_import_df)} кораблів для імпорту")
                    
                    # Map port names to IDs
                    ports_df_local = api.get_ports()
                    port_name_to_id = {}
                    if not ports_df_local.empty and "name" in ports_df_local.columns:
                        port_name_to_id = dict(zip(ports_df_local["name"], ports_df_local["id"]))
                    
                    # Map company names to IDs
                    companies_df = api.get_companies()
                    company_name_to_id = {}
                    if not companies_df.empty and "name" in companies_df.columns:
                        company_name_to_id = dict(zip(companies_df["name"], companies_df["id"]))
                    
                    default_port = st.selectbox(
                        "Порт за замовчуванням (якщо не вказано у CSV)",
                        list(port_name_to_id.keys()) if port_name_to_id else ["Немає портів"],
                        key="default_port_ships",
                    )
                    
                    if st.button("🚢 Імпортувати всі кораблі", type="primary"):
                        success_count = 0
                        error_count = 0
                        
                        progress_bar = st.progress(0)
                        status_text = st.empty()
                        
                        for idx, row in ships_import_df.iterrows():
                            try:
                                ship_name = str(row.get("name", "")).strip()
                                if not ship_name:
                                    error_count += 1
                                    continue
                                
                                ship_type = str(row.get("type", "cargo")).strip()
                                ship_country = str(row.get("country", "Unknown")).strip()
                                
                                # Resolve port
                                port_name = str(row.get("port_name", "")).strip()
                                port_id = port_name_to_id.get(port_name, port_name_to_id.get(default_port, 0))
                                
                                # Resolve company
                                company_name = str(row.get("company_name", "")).strip()
                                company_id = company_name_to_id.get(company_name, 0)
                                
                                payload = {
                                    "name": ship_name,
                                    "type": ship_type,
                                    "country": ship_country,
                                    "port_id": int(port_id) if port_id else 1,
                                    "company_id": int(company_id) if company_id else 0,
                                    "status": "docked",
                                }
                                
                                api.api_post("/api/ships", payload, success_msg="", rerun=False)
                                success_count += 1
                                
                            except Exception:
                                error_count += 1
                            
                            progress = (idx + 1) / len(ships_import_df)
                            progress_bar.progress(progress)
                            status_text.text(f"Імпортовано: {success_count}, помилок: {error_count}")
                        
                        st.success(f"✅ Імпорт завершено! Успішно: {success_count}, помилок: {error_count}")
                        if success_count > 0:
                            api.clear_all_caches()
                            st.rerun()
                        
            except Exception as e:
                st.error(f"Помилка читання CSV: {e}")

    # --------- ІМПОРТ ПОРТІВ ---------
    elif import_tab == "⚓ Імпорт портів (CSV)":
        st.markdown("### Завантажити порти з CSV файлу")
        
        st.markdown("""
        **Формат CSV:** `name,region,lat,lon`
        
        **Приклад:**
        ```
        Odesa,Europe,46.4825,30.7233
        Rotterdam,Europe,51.9244,4.4777
        Singapore,Asia,1.2897,103.8501
        New York,North America,40.6895,-74.0447
        ```
        """)
        
        uploaded_ports = st.file_uploader(
            "Виберіть CSV файл з портами",
            type=["csv"],
            key="upload_ports",
        )
        
        if uploaded_ports:
            try:
                ports_import_df = pd.read_csv(uploaded_ports)
                
                st.markdown("**Попередній перегляд:**")
                st.dataframe(ports_import_df.head(10), use_container_width=True)
                
                required_cols = ["name", "region", "lat", "lon"]
                missing = [c for c in required_cols if c not in ports_import_df.columns]
                
                if missing:
                    st.error(f"❌ Відсутні обов'язкові колонки: {', '.join(missing)}")
                else:
                    st.success(f"✅ Знайдено {len(ports_import_df)} портів для імпорту")
                    
                    if st.button("⚓ Імпортувати всі порти", type="primary"):
                        success_count = 0
                        error_count = 0
                        
                        progress_bar = st.progress(0)
                        status_text = st.empty()
                        
                        for idx, row in ports_import_df.iterrows():
                            try:
                                port_name = str(row.get("name", "")).strip()
                                if not port_name:
                                    error_count += 1
                                    continue
                                
                                payload = {
                                    "name": port_name,
                                    "region": str(row.get("region", "Unknown")).strip(),
                                    "lat": float(row.get("lat", 0.0)),
                                    "lon": float(row.get("lon", 0.0)),
                                }
                                
                                api.api_post("/api/ports", payload, success_msg="", rerun=False)
                                success_count += 1
                                
                            except Exception:
                                error_count += 1
                            
                            progress = (idx + 1) / len(ports_import_df)
                            progress_bar.progress(progress)
                            status_text.text(f"Імпортовано: {success_count}, помилок: {error_count}")
                        
                        st.success(f"✅ Імпорт завершено! Успішно: {success_count}, помилок: {error_count}")
                        if success_count > 0:
                            api.clear_all_caches()
                            st.rerun()
                        
            except Exception as e:
                st.error(f"Помилка читання CSV: {e}")

    # --------- ГЕОКОДУВАННЯ ПОРТІВ ---------
    elif import_tab == "🌍 Геокодування портів":
        st.markdown("### Автоматичне отримання координат через OpenStreetMap")
        
        st.markdown("""
        **OpenStreetMap Nominatim API** — безкоштовний сервіс для геокодування.
        
        Введи назви портів, і система автоматично знайде координати.
        """)
        
        port_names_input = st.text_area(
            "Введи назви портів (кожна назва з нового рядка)",
            placeholder="Odesa\nRotterdam\nSingapore\nNew York",
            height=150,
        )
        
        default_region = st.text_input("Регіон за замовчуванням", value="Unknown")
        
        if st.button("🌍 Знайти координати та імпортувати", type="primary"):
            if not port_names_input.strip():
                st.warning("Введи хоча б одну назву порту.")
            else:
                import requests
                from time import sleep
                
                port_lines = [line.strip() for line in port_names_input.strip().split("\n") if line.strip()]
                
                st.info(f"Знайдено {len(port_lines)} портів для геокодування...")
                
                success_count = 0
                error_count = 0
                
                progress_bar = st.progress(0)
                status_text = st.empty()
                results_container = st.container()
                
                for idx, port_name in enumerate(port_lines):
                    try:
                        status_text.text(f"Геокодування: {port_name}...")
                        
                        # Nominatim API request
                        url = "https://nominatim.openstreetmap.org/search"
                        params = {
                            "q": f"{port_name} port",
                            "format": "json",
                            "limit": 1,
                        }
                        headers = {
                            "User-Agent": "FleetManager/1.0"
                        }
                        
                        resp = requests.get(url, params=params, headers=headers, timeout=10)
                        resp.raise_for_status()
                        
                        data = resp.json()
                        
                        if data:
                            lat = float(data[0]["lat"])
                            lon = float(data[0]["lon"])
                            
                            # Create port
                            payload = {
                                "name": port_name,
                                "region": default_region,
                                "lat": lat,
                                "lon": lon,
                            }
                            
                            api.api_post("/api/ports", payload, success_msg="", rerun=False)
                            
                            with results_container:
                                st.success(f"✅ {port_name}: ({lat:.4f}, {lon:.4f})")
                            
                            success_count += 1
                        else:
                            with results_container:
                                st.warning(f"⚠️ {port_name}: не знайдено")
                            error_count += 1
                        
                        # Respect rate limit (1 req/sec for Nominatim)
                        sleep(1.1)
                        
                    except Exception as e:
                        with results_container:
                            st.error(f"❌ {port_name}: {e}")
                        error_count += 1
                    
                    progress = (idx + 1) / len(port_lines)
                    progress_bar.progress(progress)
                
                status_text.text("")
                st.success(f"🎉 Геокодування завершено! Успішно: {success_count}, помилок: {error_count}")
                
                if success_count > 0:
                    api.clear_all_caches()
                    st.rerun()

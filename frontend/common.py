from __future__ import annotations

import os
import math

import streamlit as st
import requests
import pandas as pd


### КОНФІГ
BASE_URL = os.getenv("FLEET_BASE_URL", "http://127.0.0.1:8082")
EXPORT_TOKEN = os.getenv("FLEET_EXPORT_TOKEN", "fleet-export-2025")

_SESSION = requests.Session()

TTL_SHORT = 3
TTL_MED = 5
TTL_LONG = 15


### HAVERSINE DISTANCE
def haversine_distance(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    R = 6371.0
    
    lat1_rad = math.radians(lat1)
    lon1_rad = math.radians(lon1)
    lat2_rad = math.radians(lat2)
    lon2_rad = math.radians(lon2)
    
    dlat = lat2_rad - lat1_rad
    dlon = lon2_rad - lon1_rad
    
    a = math.sin(dlat / 2)**2 + math.cos(lat1_rad) * math.cos(lat2_rad) * math.sin(dlon / 2)**2
    c = 2 * math.asin(math.sqrt(a))
    
    return R * c



### ЛОКАЛІЗАЦІЯ
TRANSLATIONS = {
    "en": {
        "app_title": "Fleet Manager",
        "backend_ok": "Backend: OK",
        "backend_unavailable": "Backend unavailable",
        "refresh": "🔄 Refresh",
        "reset_filters": "🗑️ Reset filters",
        "download_csv": "📥 Download as CSV",
        "search": "🔎 Search",
        "filters": "🔍 Filters",
        "all": "All",
        "create": "Create",
        "update": "Update",
        "delete": "Delete",
        "save": "💾 Save",
        "cancel": "Cancel",
        "edit": "✏️ Edit",
        "actions": "Actions",
        "yes": "Yes",
        "no": "No",
        
        # Navigation
        "ship_management": "Ship Management",
        "crew_people": "Crew & People",
        "company_management": "Company Management",
        "admin_data": "Admin & Data",
        "logs_analytics": "Logs & Analytics",
        
        # Logs & Analytics
        "logs_title": "Logs & Analytics",
        "action_type": "Action Type",
        "object_type": "Object",
        "all_actions": "All actions",
        "all_objects": "All objects",
        "ships": "Ships",
        "ports": "Ports",
        "companies": "Companies",
        "people": "People",
        "search_messages": "Search in messages",
        "search_placeholder": "Enter search text...",
        "importance": "Importance",
        "all_levels": "All levels",
        "information": "Information",
        "warning": "Warning",
        "error": "Error",
        "period": "📅 Period",
        "last_7_days": "Last 7 days",
        "last_30_days": "Last 30 days",
        "last_90_days": "Last 90 days",
        "custom_range": "Custom range",
        "records_count": "Records count",
        "from_date": "From",
        "to_date": "To",
        "no_records": "📭 No records for selected filters",
        "time": "Time",
        "description": "Description",
        "user": "User",
        "system": "system",
        "total_records": "📊 Total records",
        "errors": "❌ Errors",
        "warnings": "⚠️ Warnings",
        "event_history": "📜 Event History",
        "analytics": "📊 Analytics",
        "distribution_by_actions": "Distribution by action type",
        "distribution_by_importance": "Distribution by importance",
        "activity_by_days": "Activity by days",
        "no_data": "No data to display",
        "no_event_data": "No event type data",
        "no_level_data": "No importance level data",
        "no_time_data": "No time data",
        "ship_create": "Ship created",
        "ship_update": "Ship updated",
        "ship_delete": "Ship deleted",
        "port_create": "Port created",
        "port_update": "Port updated",
        "port_delete": "Port deleted",
        "company_create": "Company created",
        "company_update": "Company updated",
        "company_delete": "Company deleted",
        "person_create": "Person added",
        "person_update": "Person updated",
        "person_delete": "Person deleted",
        "other": "Other",
        "action": "Action",
        "quantity": "Quantity",
        "date": "Date",
        "events_count": "Events count",
    },
    "uk": {
        # Common
        "app_title": "Менеджер Флоту",
        "backend_ok": "Backend: OK",
        "backend_unavailable": "Backend недоступний",
        "refresh": "🔄 Оновити",
        "reset_filters": "🗑️ Скинути фільтри",
        "download_csv": "📥 Завантажити як CSV",
        "search": "🔎 Пошук",
        "filters": "🔍 Фільтри",
        "all": "Всі",
        "create": "Створити",
        "update": "Оновити",
        "delete": "Видалити",
        "save": "💾 Зберегти",
        "cancel": "Скасувати",
        "edit": "✏️ Редагувати",
        "actions": "Дії",
        "yes": "Так",
        "no": "Ні",
        
        # Navigation
        "ship_management": "Керування Кораблями",
        "crew_people": "Екіпаж і Люди",
        "company_management": "Керування Компаніями",
        "admin_data": "Адмін & Дані",
        "logs_analytics": "Логи і Аналітика",
        
        # Logs & Analytics
        "logs_title": "Логи і Аналітика",
        "action_type": "Тип дії",
        "object_type": "Об'єкт",
        "all_actions": "Всі дії",
        "all_objects": "Всі об'єкти",
        "ships": "Кораблі",
        "ports": "Порти",
        "companies": "Компанії",
        "people": "Люди",
        "search_messages": "Пошук у повідомленнях",
        "search_placeholder": "Введіть текст для пошуку...",
        "importance": "Важливість",
        "all_levels": "Всі рівні",
        "information": "Інформація",
        "warning": "Попередження",
        "error": "Помилка",
        "period": "📅 Період",
        "last_7_days": "Останні 7 днів",
        "last_30_days": "Останні 30 днів",
        "last_90_days": "Останні 90 днів",
        "custom_range": "Вибрати діапазон",
        "records_count": "Кількість записів",
        "from_date": "З дати",
        "to_date": "До дати",
        "no_records": "📭 Немає записів для обраних фільтрів",
        "time": "Час",
        "description": "Опис",
        "user": "Користувач",
        "system": "система",
        "total_records": "📊 Всього записів",
        "errors": "❌ Помилок",
        "warnings": "⚠️ Попереджень",
        "event_history": "📜 Історія подій",
        "analytics": "📊 Аналітика",
        "distribution_by_actions": "Розподіл за типом дій",
        "distribution_by_importance": "Розподіл за важливістю",
        "activity_by_days": "Активність по днях",
        "no_data": "Немає даних для відображення",
        "no_event_data": "Немає даних про типи подій",
        "no_level_data": "Немає даних про рівні важливості",
        "no_time_data": "Немає даних про час подій",
        "ship_create": "Створення корабля",
        "ship_update": "Оновлення корабля",
        "ship_delete": "Видалення корабля",
        "port_create": "Створення порту",
        "port_update": "Оновлення порту",
        "port_delete": "Видалення порту",
        "company_create": "Створення компанії",
        "company_update": "Оновлення компанії",
        "company_delete": "Видалення компанії",
        "person_create": "Додано людину",
        "person_update": "Оновлено людину",
        "person_delete": "Видалено людину",
        "other": "інше",
        "action": "Дія",
        "quantity": "Кількість",
        "date": "Дата",
        "events_count": "Кількість подій",
    }
}

def get_lang():
    """Always return English language."""
    return "en"

def t(key: str) -> str:
    """Get translation for key in English."""
    return TRANSLATIONS.get("en", {}).get(key, key)


### THEME / LAYOUT
def inject_theme():
    st.markdown(
        """
        <style>
        :root {
            --fm-bg: #0f172a;
            --fm-panel: #111827;
            --fm-panel-alt: #0b1220;
            --fm-border: #1f2937;
            --fm-accent: #38bdf8;
            --fm-accent-2: #22d3ee;
            --fm-text-sub: #9ca3af;
        }
        .block-container { padding-top: 1.3rem; }
        .fm-hero {
            padding: 1rem 1.25rem;
            border-radius: 12px;
            background: radial-gradient(circle at 10% 20%, rgba(56,189,248,0.12), transparent 35%),
                        radial-gradient(circle at 80% 10%, rgba(34,211,238,0.12), transparent 32%),
                        linear-gradient(135deg, #0b1220, #0f172a 60%);
            border: 1px solid var(--fm-border);
        }
        .fm-hero h1 { margin-bottom: 0.3rem; }
        .fm-hero p { color: var(--fm-text-sub); margin-bottom: 0.4rem; }
        .fm-chip {
            display: inline-flex;
            align-items: center;
            gap: 0.35rem;
            padding: 0.35rem 0.75rem;
            border-radius: 999px;
            border: 1px solid var(--fm-border);
            background: rgba(56,189,248,0.08);
            color: #e5e7eb;
            font-size: 0.88rem;
        }
        .fm-card {
            padding: 0.75rem 0.9rem;
            border-radius: 12px;
            border: 1px solid var(--fm-border);
            background: var(--fm-panel);
        }
        .fm-section-title { margin-bottom: 0.35rem; }
        .stMetric { background: var(--fm-panel); padding: 0.6rem 0.8rem; border-radius: 10px; border: 1px solid var(--fm-border); }
        .stMetric label, .stMetric [data-testid="stMetricDelta"] { color: var(--fm-text-sub); }
        .st-expander { border: 1px solid var(--fm-border) !important; border-radius: 10px !important; }
        .stDownloadButton button { width: 100%; }
        </style>
        """,
        unsafe_allow_html=True,
    )


### DATAFRAME HELPERS
def df_1based(df: pd.DataFrame) -> pd.DataFrame:
    if df is None or df.empty:
        return df
    df = df.copy()
    df.index = range(1, len(df) + 1)
    df.index.name = "#"
    return df


### КЕШ / ОЧИЩЕННЯ
def clear_all_caches():
    try:
        st.cache_data.clear()
        return
    except Exception:
        pass

    for fn in [
        get_ports,
        get_ship_types,
        get_ships,
        get_companies,
        get_people,
        get_ship_crew,
        get_all_active_person_ids,
        get_company_ports,
        get_active_ship_map,
        get_active_assignments,
    ]:
        try:
            fn.clear()
        except Exception:
            continue


### API ХЕЛПЕРИ (CRUD)
def _url(path: str) -> str:
    return BASE_URL + path


def _handle_api_error(resp: requests.Response, action: str):
    try:
        data = resp.json()
        msg = data.get("error") or data.get("details") or resp.text
    except Exception:
        msg = resp.text

    st.error(f"{action} failed: {msg} (Code: {resp.status_code})")
    return None


def _after_success(success_msg: str, rerun: bool = True):
    clear_all_caches()
    st.session_state["last_success"] = success_msg
    if rerun:
        st.rerun()


def api_get(path: str, *, expect_json: bool = True):
    url = _url(path)
    resp = _SESSION.get(url, timeout=5)
    resp.raise_for_status()
    if not resp.text:
        return None

    if not expect_json:
        return resp.text

    try:
        return resp.json()
    except ValueError:
        return {"raw": resp.text}


def api_post(path: str, payload: dict, success_msg: str, rerun: bool = True):
    url = _url(path)
    resp = _SESSION.post(url, json=payload, timeout=5)
    if not resp.ok:
        return _handle_api_error(resp, "Create")

    data = resp.json() if resp.text else None
    _after_success(success_msg, rerun=rerun)
    return data


def api_put(path: str, payload: dict, success_msg: str, rerun: bool = True):
    """ОНОВЛЕННЯ (UPDATE)."""
    url = _url(path)
    resp = _SESSION.put(url, json=payload, timeout=5)
    if not resp.ok:
        return _handle_api_error(resp, "Update")

    data = resp.json() if resp.text else None
    _after_success(success_msg, rerun=rerun)
    return data


def api_del(path: str, success_msg: str, rerun: bool = True):
    """ВИДАЛЕННЯ (DELETE)."""
    url = _url(path)
    resp = _SESSION.delete(url, timeout=5)
    if not resp.ok:
        return _handle_api_error(resp, "Delete")

    _after_success(success_msg, rerun=rerun)
    return True


### КЕШОВАНІ ЧИТАННЯ
@st.cache_data(ttl=TTL_LONG)
def get_ports() -> pd.DataFrame:
    data = api_get("/api/ports") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=TTL_LONG)
def get_ship_types() -> pd.DataFrame:
    data = api_get("/api/ship-types") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=TTL_MED)
def get_ships() -> pd.DataFrame:
    data = api_get("/api/ships") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=TTL_LONG)
def get_companies() -> pd.DataFrame:
    data = api_get("/api/companies") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=TTL_MED)
def get_people() -> pd.DataFrame:
    data = api_get("/api/people") or []
    return pd.DataFrame(data)


def get_ship_crew(ship_id: int) -> pd.DataFrame:
    """Get ship crew without caching to ensure fresh data"""
    if not ship_id:
        return pd.DataFrame()
    data = api_get(f"/api/ships/{ship_id}/crew") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=TTL_MED)
def get_company_ports(company_id: int) -> pd.DataFrame:
    if not company_id:
        return pd.DataFrame()
    data = api_get(f"/api/companies/{company_id}/ports") or []
    return pd.DataFrame(data)


### АКТИВНІ ПРИЗНАЧЕННЯ
@st.cache_data(ttl=TTL_SHORT)
def get_active_assignments() -> pd.DataFrame:
    ships_df = get_ships()
    if ships_df.empty or "id" not in ships_df.columns:
        return pd.DataFrame(columns=["person_id", "ship_id"])

    rows: list[pd.DataFrame] = []
    for ship_id in ships_df["id"].dropna().astype(int).tolist():
        try:
            crew_df = get_ship_crew(ship_id)
        except Exception:
            continue

        if crew_df.empty or "person_id" not in crew_df.columns:
            continue

        if "end_utc" in crew_df.columns:
            crew_df = crew_df[crew_df["end_utc"].isna()].copy()

        if crew_df.empty:
            continue

        crew_df["ship_id"] = int(ship_id)
        rows.append(crew_df)

    if not rows:
        return pd.DataFrame(columns=["person_id", "ship_id"])

    merged = pd.concat(rows, ignore_index=True)

    for col in ["person_id", "ship_id"]:
        if col not in merged.columns:
            merged[col] = pd.Series(dtype="int64")

    return merged


@st.cache_data(ttl=TTL_SHORT)
def get_all_active_person_ids() -> set[int]:
    df = get_active_assignments()
    if df.empty or "person_id" not in df.columns:
        return set()

    ids: list[int] = []
    for v in df["person_id"].dropna().tolist():
        try:
            ids.append(int(v))
        except Exception:
            continue
    return set(ids)


@st.cache_data(ttl=TTL_SHORT)
def get_active_ship_map() -> dict[int, int]:
    df = get_active_assignments()
    if df.empty or not {"person_id", "ship_id"}.issubset(df.columns):
        return {}

    result: dict[int, int] = {}
    for _, row in df.iterrows():
        try:
            pid = int(row["person_id"])
            sid = int(row["ship_id"])
            result[pid] = sid
        except Exception:
            continue

    return result


@st.cache_data(ttl=5)
def get_health() -> dict | None:
    """Return backend health JSON or None on error."""
    try:
        return api_get("/health") or None
    except Exception:
        return None


### ХЕЛПЕРИ ДЛЯ UI
def get_name_map(df: pd.DataFrame, id_col: str = "id", name_col: str = "name") -> dict:
    if df.empty or id_col not in df.columns or name_col not in df.columns:
        return {}
    try:
        return pd.Series(df[name_col].values, index=df[id_col]).to_dict()
    except Exception:
        return {}


def get_ship_name_map() -> dict[int, str]:
    ships = get_ships()
    if ships.empty or "id" not in ships.columns:
        return {}

    def make_label(row: pd.Series) -> str:
        name = row.get("name", "")
        sid = row.get("id", "")
        stype = row.get("type", "")
        return f"{name} (id={sid}, type={stype})"

    out: dict[int, str] = {}
    for _, row in ships.iterrows():
        try:
            sid = int(row["id"])
            out[sid] = make_label(row)
        except Exception:
            continue
    return out


def get_person_name_map() -> dict[int, str]:
    people = get_people()
    if people.empty or "id" not in people.columns:
        return {}

    def make_label(row: pd.Series) -> str:
        name = row.get("full_name", "")
        pid = row.get("id", "")
        rank = row.get("rank", "")
        return f"{name} (id={pid}, rank={rank})"

    out: dict[int, str] = {}
    for _, row in people.iterrows():
        try:
            pid = int(row["id"])
            out[pid] = make_label(row)
        except Exception:
            continue
    return out


### STICKY TABS
def sticky_tabs(labels: list[str], key: str, default: int = 0) -> str:
    if not labels:
        return ""

    state_key = f"sticky_tab::{key}"

    if state_key not in st.session_state:
        if 0 <= default < len(labels):
            st.session_state[state_key] = labels[default]
        else:
            st.session_state[state_key] = labels[0]

    current = st.session_state.get(state_key, labels[0])
    if current not in labels:
        current = labels[0]

    idx = labels.index(current)

    choice = st.radio(
        label=f"tabs_{key}",
        options=labels,
        index=idx,
        horizontal=True,
        key=state_key,
        label_visibility="collapsed",
    )


    return choice


def api_export_json():
    """Fetch full data export with token auth."""
    url = _url(f"/api/export?token={EXPORT_TOKEN}")
    resp = _SESSION.get(url, timeout=10)
    resp.raise_for_status()
    return resp.json()


def api_export_logs_csv(*, event_type: str = "", entity: str = "", entity_id: str = "", since: str = "", until: str = "") -> str:
    """Fetch logs CSV with token auth and optional filters."""
    params = {"token": EXPORT_TOKEN}
    if event_type:
        params["event_type"] = event_type
    if entity:
        params["entity"] = entity
    if entity_id:
        params["entity_id"] = entity_id
    if since:
        params["since"] = since
    if until:
        params["until"] = until
    
    url = _url("/api/logs.csv")
    resp = _SESSION.get(url, params=params, timeout=15)
    resp.raise_for_status()
    return resp.text


import streamlit as st
import requests
import pandas as pd
from datetime import datetime, timezone

# ================== КОНФІГ ==================
BASE_URL = "http://127.0.0.1:8081"

# ================== ОЧИЩЕННЯ КЕШУ ==================


def clear_all_caches():
    """
    Викликається після кожної POST/PUT/DELETE дії,
    щоб змусити UI оновити дані з сервера.
    """
    get_ports.clear()
    get_ship_types.clear()
    get_ships.clear()
    get_companies.clear()
    get_people.clear()
    get_ship_crew.clear()
    get_all_active_person_ids.clear()
    get_company_ports.clear()
    get_active_ship_map.clear()


def df_1based(df: pd.DataFrame) -> pd.DataFrame:
    """
    Повертає копію DataFrame з індексом, що починається з 1.
    Використовуємо перед st.dataframe, щоб рядки нумерувалися 1,2,3...
    """
    if df is None or df.empty:
        return df
    df = df.copy()
    df.index = range(1, len(df) + 1)
    df.index.name = "#"   # просто гарний заголовок для індекса
    return df

# ================== API ХЕЛПЕРИ (CRUD) ==================


def _handle_api_error(resp: requests.Response, action: str):
    """Внутрішній хелпер для обробки помилок API."""
    try:
        data = resp.json()
        msg = data.get("error") or data.get("details") or resp.text
    except requests.exceptions.JSONDecodeError:
        msg = resp.text
    st.error(f"{action} failed: {msg} (Code: {resp.status_code})")
    return None


def _after_success(success_msg: str, rerun: bool = True):
    """
    Викликаємо після успішного POST/PUT/DELETE:
    - очищаємо кеш;
    - кладемо повідомлення в session_state;
    - при потребі робимо st.rerun().
    """
    clear_all_caches()
    st.session_state["last_success"] = success_msg
    # if rerun:
    #     st.rerun()


def api_get(path: str):
    url = BASE_URL + path
    resp = requests.get(url, timeout=5)
    resp.raise_for_status()
    if not resp.text:
        return None
    return resp.json()


def api_post(path: str, payload: dict, success_msg: str, rerun: bool = True):
    """СТВОРЕННЯ (CREATE)."""
    url = BASE_URL + path
    resp = requests.post(url, json=payload, timeout=5)
    if not resp.ok:
        return _handle_api_error(resp, "Create")

    data = resp.json() if resp.text else None
    _after_success(success_msg, rerun=rerun)
    return data  # фактично не повернеться, якщо rerun=True


def api_put(path: str, payload: dict, success_msg: str, rerun: bool = True):
    """ОНОВЛЕННЯ (UPDATE)."""
    url = BASE_URL + path
    resp = requests.put(url, json=payload, timeout=5)
    if not resp.ok:
        return _handle_api_error(resp, "Update")

    data = resp.json() if resp.text else None
    _after_success(success_msg, rerun=rerun)
    return data


def api_del(path: str, success_msg: str, rerun: bool = True):
    """ВИДАЛЕННЯ (DELETE)."""
    url = BASE_URL + path
    resp = requests.delete(url, timeout=5)
    if not resp.ok:
        return _handle_api_error(resp, "Delete")

    _after_success(success_msg, rerun=rerun)
    return True

# ================== КЕШОВАНІ ЧИТАННЯ ==================
# Всі GET-запити кешуються, щоб не навантажувати бекенд


@st.cache_data(ttl=15)
def get_ports() -> pd.DataFrame:
    data = api_get("/api/ports") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=15)
def get_ship_types() -> pd.DataFrame:
    data = api_get("/api/ship-types") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=5)
def get_ships() -> pd.DataFrame:
    data = api_get("/api/ships") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=15)
def get_companies() -> pd.DataFrame:
    data = api_get("/api/companies") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=5)
def get_people() -> pd.DataFrame:
    data = api_get("/api/people") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=3)
def get_ship_crew(ship_id: int) -> pd.DataFrame:
    if not ship_id:
        return pd.DataFrame()
    data = api_get(f"/api/ships/{ship_id}/crew") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=5)
def get_company_ports(company_id: int) -> pd.DataFrame:
    if not company_id:
        return pd.DataFrame()
    data = api_get(f"/api/companies/{company_id}/ports") or []
    return pd.DataFrame(data)


@st.cache_data(ttl=3)
def get_all_active_person_ids() -> set[int]:
    """Будуємо множину person_id, які зараз у якійсь команді (end_utc is null)."""
    ships_df = get_ships()
    active_ids: set[int] = set()
    if ships_df.empty or "id" not in ships_df.columns:
        return active_ids

    for ship_id in ships_df["id"].tolist():
        try:
            crew_df = get_ship_crew(ship_id)
        except Exception:
            continue
        if crew_df.empty or "person_id" not in crew_df.columns:
            continue

        # end_utc == null -> активне призначення
        mask_active = crew_df["end_utc"].isna()
        active_ids.update(
            crew_df.loc[mask_active, "person_id"]
            .dropna()
            .astype(int)
            .tolist()
        )
    return active_ids


@st.cache_data(ttl=3)
def get_active_ship_map() -> dict[int, int]:
    """
    Повертає словник {person_id: ship_id} для всіх
    АКТИВНИХ призначень (end_utc is NULL).
    """
    ships_df = get_ships()
    result: dict[int, int] = {}

    if ships_df.empty or "id" not in ships_df.columns:
        return result

    for ship_id in ships_df["id"].tolist():
        try:
            crew_df = get_ship_crew(ship_id)
        except Exception:
            continue

        if crew_df.empty or "person_id" not in crew_df.columns:
            continue

        mask_active = crew_df["end_utc"].isna()
        if "person_id" not in crew_df.columns:
            continue

        active_person_ids = (
            crew_df.loc[mask_active, "person_id"]
            .dropna()
            .astype(int)
            .tolist()
        )

        for pid in active_person_ids:
            result[int(pid)] = int(ship_id)

    return result

# ================== ХЕЛПЕРИ ДЛЯ UI ==================


def get_name_map(df: pd.DataFrame, id_col: str = 'id', name_col: str = 'name') -> dict:
    """Створює словник {id: name} з DataFrame."""
    if df.empty or id_col not in df.columns or name_col not in df.columns:
        return {}
    return pd.Series(df[name_col].values, index=df[id_col]).to_dict()


def get_ship_name_map() -> dict:
    ships = get_ships()
    if ships.empty or "id" not in ships.columns:
        return {}

    def make_label(row):
        return f"{row['name']} (id={row['id']}, type={row['type']})"

    return {row['id']: make_label(row) for _, row in ships.iterrows()}


def get_person_name_map() -> dict:
    people = get_people()
    if people.empty or "id" not in people.columns:
        return {}

    def make_label(row):
        return f"{row['full_name']} (id={row['id']}, rank={row['rank']})"

    return {row['id']: make_label(row) for _, row in people.iterrows()}

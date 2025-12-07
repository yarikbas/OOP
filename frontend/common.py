# frontend/common.py
from __future__ import annotations

import os
from typing import Any

import streamlit as st
import requests
import pandas as pd


# ================== КОНФІГ ==================
BASE_URL = os.getenv("FLEET_BASE_URL", "http://127.0.0.1:8082")

# Реюз TCP-з'єднань
_SESSION = requests.Session()

# TTL централізовано
TTL_SHORT = 3
TTL_MED = 5
TTL_LONG = 15


# ================== DATAFRAME HELPERS ==================
def df_1based(df: pd.DataFrame) -> pd.DataFrame:
    """
    Повертає копію DataFrame з індексом, що починається з 1.
    Використовуємо перед st.dataframe, щоб рядки нумерувалися 1,2,3...
    """
    if df is None or df.empty:
        return df
    df = df.copy()
    df.index = range(1, len(df) + 1)
    df.index.name = "#"
    return df


# ================== КЕШ / ОЧИЩЕННЯ ==================
def clear_all_caches():
    """
    Викликається після кожної POST/PUT/DELETE дії,
    щоб змусити UI оновити дані з сервера.
    """
    # Найнадійніше: очистка всього cache_data в межах додатку
    try:
        st.cache_data.clear()
        return
    except Exception:
        pass

    # Fallback (на випадок змін у Streamlit)
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
            fn.clear()  # type: ignore[attr-defined]
        except Exception:
            continue


# ================== API ХЕЛПЕРИ (CRUD) ==================
def _url(path: str) -> str:
    return BASE_URL + path


def _handle_api_error(resp: requests.Response, action: str):
    """Внутрішній хелпер для обробки помилок API."""
    try:
        data = resp.json()
        msg = data.get("error") or data.get("details") or resp.text
    except Exception:
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
        # Деякі ендпоінти (наприклад старий /health) могли повертати plain text.
        # Повертаємо сирий текст, щоб не падати на JSONDecodeError.
        return {"raw": resp.text}


def api_post(path: str, payload: dict, success_msg: str, rerun: bool = True):
    """СТВОРЕННЯ (CREATE)."""
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


# ================== КЕШОВАНІ ЧИТАННЯ ==================
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


@st.cache_data(ttl=TTL_SHORT)
def get_ship_crew(ship_id: int) -> pd.DataFrame:
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


# ================== АКТИВНІ ПРИЗНАЧЕННЯ ==================
@st.cache_data(ttl=TTL_SHORT)
def get_active_assignments() -> pd.DataFrame:
    """
    Повертає DataFrame активних призначень з колонками:
    person_id, ship_id, ...
    Будуємо 1 раз і використовуємо в кількох місцях.
    """
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

        # активні: end_utc == null (якщо колонка є)
        if "end_utc" in crew_df.columns:
            crew_df = crew_df[crew_df["end_utc"].isna()].copy()

        if crew_df.empty:
            continue

        crew_df["ship_id"] = int(ship_id)
        rows.append(crew_df)

    if not rows:
        return pd.DataFrame(columns=["person_id", "ship_id"])

    merged = pd.concat(rows, ignore_index=True)

    # Гарантуємо потрібні колонки
    for col in ["person_id", "ship_id"]:
        if col not in merged.columns:
            merged[col] = pd.Series(dtype="int64")

    return merged


@st.cache_data(ttl=TTL_SHORT)
def get_all_active_person_ids() -> set[int]:
    """Будуємо множину person_id, які зараз у якійсь команді."""
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
    """
    Повертає словник {person_id: ship_id}
    для всіх АКТИВНИХ призначень.
    """
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


# ================== ХЕЛПЕРИ ДЛЯ UI ==================
def get_name_map(df: pd.DataFrame, id_col: str = "id", name_col: str = "name") -> dict:
    """Створює словник {id: name} з DataFrame."""
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


# ================== STICKY TABS ==================
def sticky_tabs(labels: list[str], key: str, default: int = 0) -> str:
    """
    Липкі вкладки-радіо, які:
    - НЕ скидаються при selectbox/checkbox
    - НЕ ламають session_state
    - переживають rerun

    Повертає назву активної вкладки.

    Використання:
        tab = api.sticky_tabs(["A", "B", "C"], "unique_key")
        if tab == "A":
            ...
    """
    if not labels:
        return ""

    state_key = f"sticky_tab::{key}"

    # Ініціалізуємо ДО створення віджета
    if state_key not in st.session_state:
        if 0 <= default < len(labels):
            st.session_state[state_key] = labels[default]
        else:
            st.session_state[state_key] = labels[0]

    current = st.session_state.get(state_key, labels[0])
    if current not in labels:
        current = labels[0]

    idx = labels.index(current)

    # key віджета = state_key
    # Streamlit сам оновить st.session_state[state_key]
    choice = st.radio(
        label="",
        options=labels,
        index=idx,
        horizontal=True,
        key=state_key,
        label_visibility="collapsed",
    )

    return choice

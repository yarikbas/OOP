from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api

# ================== КОНФІГ додатку ==================
st.set_page_config(
    page_title="Fleet Manager Dashboard",
    page_icon="🚢",
    layout="wide",
)

# ================== ХЕЛПЕРИ ==================
def safe_cols(df: pd.DataFrame, cols: list[str]) -> list[str]:
    return [c for c in cols if c in df.columns]


def dataframe_1based(df: pd.DataFrame):
    st.dataframe(api.df_1based(df), use_container_width=True)


@st.cache_data(ttl=10)
def load_all():
    # /health може бути json або plaintext у старих версіях
    health_ok = False

    try:
        health = api.api_get("/health")
        if isinstance(health, dict) and health.get("status") == "ok":
            health_ok = True
    except Exception:
        pass

    if not health_ok:
        try:
            health_text = api.api_get("/health", expect_json=False)
            if isinstance(health_text, str) and health_text.strip().upper() == "OK":
                health_ok = True
        except Exception:
            pass

    if not health_ok:
        raise RuntimeError("Backend /health недоступний або повертає неочікувану відповідь")

    ports_df = api.get_ports()
    ships_df = api.get_ships()
    people_df = api.get_people()
    companies_df = api.get_companies()
    types_df = api.get_ship_types()

    return ports_df, ships_df, people_df, companies_df, types_df


# ================== FLASH ==================
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

# ================== LOAD ==================
try:
    ports_df, ships_df, people_df, companies_df, types_df = load_all()
except Exception as e:
    st.error(f"💥 Backend недоступний за адресою {api.BASE_URL}")
    st.error(f"Деталі: {e}")
    st.stop()

# ================== ACTIVE SHIPS ==================
active_ships_df = ships_df.copy()
if "status" in active_ships_df.columns:
    active_ships_df = active_ships_df[active_ships_df["status"] != "departed"].copy()

# ================== HEADER ==================
st.title("🚢 Fleet Manager Dashboard")
st.markdown("Огляд стану портів, флоту, екіпажу та компаній.")

c1, c2, c3, c4, c5 = st.columns(5)
c1.metric("⚓ Порти", len(ports_df))
c2.metric("📋 Типи кораблів", len(types_df))
c3.metric("🚢 Кораблі (в портах)", len(active_ships_df))
c4.metric("🧑‍✈️ Персонал", len(people_df))
c5.metric("🏢 Компанії", len(companies_df))

st.markdown("---")

# ================== GUARD ==================
if ports_df.empty or "name" not in ports_df.columns:
    st.warning("Немає жодного порту в БД. Додайте порти на сторінці '⚙️ Admin'.")
    st.stop()

# ================== ВИБІР ПОРТУ ==================
port_names = ports_df["name"].dropna().astype(str).tolist()

default_index = 0
if "selected_port" in st.session_state:
    try:
        default_index = port_names.index(st.session_state["selected_port"])
    except ValueError:
        default_index = 0

col_info, col_map = st.columns([2, 1.4])

with col_info:
    st.subheader("Інформація по порту")

    selected_port_name = st.selectbox(
        "Виберіть порт",
        port_names,
        index=default_index,
        key="selected_port",
        help="Списки нижче фільтруються за цим портом.",
    )

    sel_port_row = ports_df[ports_df["name"] == selected_port_name].iloc[0]
    sel_port_id = int(sel_port_row.get("id", 0))

    st.caption(
        f"Обраний порт: **{selected_port_name}** "
        f"(id={sel_port_id}, регіон: {sel_port_row.get('region', '')})"
    )

    ships_in_port = pd.DataFrame()
    if {"port_id", "id"}.issubset(active_ships_df.columns):
        ships_in_port = active_ships_df[active_ships_df["port_id"] == sel_port_id].copy()

    companies_in_port = pd.DataFrame()
    if not ships_in_port.empty and "company_id" in ships_in_port.columns and "id" in companies_df.columns:
        ids = (
            ships_in_port["company_id"]
            .dropna()
            .astype(int, errors="ignore")
            .unique()
            .tolist()
        )
        ids = [cid for cid in ids if isinstance(cid, int) and cid > 0]
        if ids:
            companies_in_port = companies_df[companies_df["id"].isin(ids)].copy()

    tab = api.sticky_tabs(
        ["🚢 Кораблі в цьому порту", "🏢 Компанії в порту", "🌍 Всі кораблі"],
        "dashboard_port_tabs",
    )

    if tab == "🚢 Кораблі в цьому порту":
        if ships_in_port.empty:
            st.info("У цьому порту зараз немає кораблів.")
        else:
            view_cols = safe_cols(ships_in_port, ["id", "name", "type", "country", "status", "company_id"])
            dataframe_1based(ships_in_port[view_cols])

    elif tab == "🏢 Компанії в порту":
        if companies_in_port.empty:
            st.info("У цьому порту нема активних кораблів компаній.")
        else:
            view_cols = safe_cols(companies_in_port, ["id", "name"])
            dataframe_1based(companies_in_port[view_cols])

    elif tab == "🌍 Всі кораблі":
        all_view_cols = safe_cols(ships_df, ["id", "name", "type", "country", "status", "port_id", "company_id"])
        if all_view_cols:
            dataframe_1based(ships_df[all_view_cols])
        else:
            st.info("Немає даних про кораблі для відображення.")

with col_map:
    st.subheader("Карта портів")

    if {"lat", "lon"}.issubset(ports_df.columns):
        ports_for_map = ports_df.rename(columns={"lat": "latitude", "lon": "longitude"})
        st.map(ports_for_map[["latitude", "longitude"]], use_container_width=True)
    else:
        st.error("У таблиці портів немає координат lat/lon.")

st.markdown("---")
st.caption(
    "Для CRUD-управління портами, кораблями, компаніями, екіпажем "
    "скористайтесь сторінками в бічному меню."
)

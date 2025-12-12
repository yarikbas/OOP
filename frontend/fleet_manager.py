from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api

### APP CONFIG
st.set_page_config(
    page_title="Fleet Manager Dashboard",
    page_icon="🚢",
    layout="wide",
)

api.inject_theme()

### HELPERS
def safe_cols(df: pd.DataFrame, cols: list[str]) -> list[str]:
    return [c for c in cols if c in df.columns]


def dataframe_1based(df: pd.DataFrame):
    df = api.df_1based(df)
    try:
        st.dataframe(df, width="stretch")
    except TypeError:
        st.dataframe(df, width="stretch")


def map_stretch(df: pd.DataFrame):
    try:
        st.map(df, width="stretch")
    except TypeError:
        st.map(df, use_container_width=True)


@st.cache_data(ttl=10)
def load_all():
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
        raise RuntimeError("Backend /health is unavailable or returns unexpected response")

    ports_df = api.get_ports()
    ships_df = api.get_ships()
    people_df = api.get_people()
    companies_df = api.get_companies()
    types_df = api.get_ship_types()

    return ports_df, ships_df, people_df, companies_df, types_df


### FLASH
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

### LOAD
try:
    ports_df, ships_df, people_df, companies_df, types_df = load_all()
except Exception as e:
    st.error(f"💥 Backend unavailable at {api.BASE_URL}")
    st.error(f"Details: {e}")
    st.stop()

health = api.get_health()

### ACTIVE SHIPS
active_ships_df = ships_df.copy()
if "status" in active_ships_df.columns:
    active_ships_df = active_ships_df[active_ships_df["status"] != "departed"].copy()

### HEADER
with st.container():
    st.markdown(
        f"""
        <div class="fm-hero">
            <h1>🚢 Fleet Manager Dashboard</h1>
            <p>Control your fleet, people, and logs in a single window.</p>
        </div>
        """,
        unsafe_allow_html=True,
    )

st.markdown("")
c1, c2, c3, c4, c5 = st.columns(5)
c1.metric("⚓ Ports", len(ports_df))
c2.metric("📋 Ship Types", len(types_df))
c3.metric("🚢 Ships (docked)", len(active_ships_df))
c4.metric("🧑‍✈️ Personnel", len(people_df))
c5.metric("🏢 Companies", len(companies_df))

st.markdown("---")

### GUARD
if ports_df.empty or "name" not in ports_df.columns:
    st.warning("No ports in database. Add ports on '⚙️ Admin' page.")
    st.stop()

### PORT SELECTION
port_names = ports_df["name"].dropna().astype(str).tolist()

default_index = 0
if "selected_port" in st.session_state:
    try:
        default_index = port_names.index(st.session_state["selected_port"])
    except ValueError:
        default_index = 0

col_info, col_map = st.columns([2, 1.4])

with col_info:
    st.subheader("Port Information")

    selected_port_name = st.selectbox(
        "Select Port",
        port_names,
        index=default_index,
        key="selected_port",
        help="Lists below are filtered by this port.",
    )

    sel_port_row = ports_df[ports_df["name"] == selected_port_name].iloc[0]
    sel_port_id = int(sel_port_row.get("id", 0))

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

    cA, cB, cC = st.columns(3)
    cA.metric("Ships in Port", len(ships_in_port))
    cB.metric("Companies in Port", len(companies_in_port))
    if not ships_in_port.empty and "status" in ships_in_port.columns:
        top_status = ships_in_port["status"].mode()[0] if not ships_in_port.empty else "—"
        cC.metric("Most Common Status", top_status)
    else:
        cC.metric("Most Common Status", "—")

    st.caption(
        f"Selected port: **{selected_port_name}** "
        f"(id={sel_port_id}, region: {sel_port_row.get('region', '')})"
    )

    tab = api.sticky_tabs(
        ["🚢 Ships in This Port", "🏢 Companies in Port", "🌍 All Ships"],
        "dashboard_port_tabs",
    )

    if tab == "🚢 Ships in This Port":
        if ships_in_port.empty:
            st.info("No ships currently in this port.")
        else:
            view_cols = safe_cols(ships_in_port, ["id", "name", "type", "country", "status", "company_id"])
            dataframe_1based(ships_in_port[view_cols])

    elif tab == "🏢 Companies in Port":
        if companies_in_port.empty:
            st.info("No active company ships in this port.")
        else:
            view_cols = safe_cols(companies_in_port, ["id", "name"])
            dataframe_1based(companies_in_port[view_cols])

    elif tab == "🌍 All Ships":
        all_view_cols = safe_cols(ships_df, ["id", "name", "type", "country", "status", "port_id", "company_id"])
        if all_view_cols:
            dataframe_1based(ships_df[all_view_cols])
        else:
            st.info("No ship data to display.")

with col_map:
    st.subheader("Ports Map")

    if {"lat", "lon"}.issubset(ports_df.columns):
        ports_for_map = ports_df.rename(columns={"lat": "latitude", "lon": "longitude"})
        map_stretch(ports_for_map[["latitude", "longitude"]])
    else:
        st.error("No lat/lon coordinates in ports table.")

st.markdown("---")
st.caption(
    "For CRUD management of ports, ships, companies, and crew, "
    "use the pages in the sidebar menu."
)

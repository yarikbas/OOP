from __future__ import annotations

import json
from datetime import datetime, timedelta
import streamlit as st
import pandas as pd

from common import api_get, df_1based, t, language_selector


st.set_page_config(page_title="Logs & Analytics", page_icon="📊", layout="wide")
from common import inject_theme
inject_theme()

# Sidebar / app identity
st.sidebar.title(f"🚢 {t('app_title')}")
st.sidebar.caption(t("logs_analytics"))

# Language selector
language_selector()

# Centered page title
col_l, col_c, col_r = st.columns([1, 3, 1])
with col_c:
    st.title(t("logs_title"))

today = datetime.utcnow().date()
default_since = today - timedelta(days=7)

with st.expander(t("filters"), expanded=True):
    r1c1, r1c2 = st.columns([2, 2])
    
    action_options = [t("all_actions"), t("create"), t("update"), t("delete")]
    action_type = r1c1.selectbox(
        t("action_type"),
        options=action_options,
        index=0,
        key="logs_action"
    )
    
    object_options = [t("all_objects"), t("ships"), t("ports"), t("companies"), t("people")]
    object_type = r1c2.selectbox(
        t("object_type"),
        options=object_options,
        index=0,
        key="logs_object"
    )
    
    # Map user-friendly names to backend values
    action_map = {
        t("create"): ".create",
        t("update"): ".update",
        t("delete"): ".delete"
    }
    object_map = {
        t("ships"): "ship",
        t("ports"): "port",
        t("companies"): "company",
        t("people"): "person"
    }
    
    event_type = action_map.get(action_type, "")
    entity = object_map.get(object_type, "")
    entity_id = ""  # Hidden from user
    
    r2c1, r2c2 = st.columns([2, 2])
    message_sub = r2c1.text_input(
        t("search_messages"),
        value=st.session_state.get("logs_msg", ""),
        key="logs_msg",
        placeholder=t("search_placeholder")
    )
    
    level_options = [t("all_levels"), t("information"), t("warning"), t("error")]
    level_display = r2c2.selectbox(
        t("importance"),
        options=level_options,
        index=0,
        key="logs_level_display"
    )
    
    # Map display level to backend
    level_map = {
        t("information"): "INFO",
        t("warning"): "WARN",
        t("error"): "ERROR"
    }
    level = level_map.get(level_display, "")

    r3c1, r3c2 = st.columns([2, 2])
    period_options = [t("last_7_days"), t("last_30_days"), t("last_90_days"), t("custom_range")]
    period = r3c1.selectbox(
        t("period"),
        period_options,
        index=0,
        key="logs_period",
    )
    page_size = int(r3c2.number_input(t("records_count"), min_value=10, max_value=500, value=100, step=10))
    page = 0  # Always show first page for simplicity

    if period == t("custom_range"):
        d1, d2 = st.columns(2)
        since_dt = d1.date_input(t("from_date"), value=default_since, key="logs_since")
        until_dt = d2.date_input(t("to_date"), value=today, key="logs_until")
    else:
        days_map = {
            t("last_7_days"): 7,
            t("last_30_days"): 30,
            t("last_90_days"): 90
        }
        days = days_map.get(period, 7)
        since_dt = today - timedelta(days=days)
        until_dt = today

    c_btn1, c_btn2, _ = st.columns([1, 1, 4])
    refresh = c_btn1.button(t("refresh"), type="primary", use_container_width=True)
    if c_btn2.button(t("reset_filters"), use_container_width=True):
        # Clear all filter keys from session state
        keys_to_clear = ["logs_action", "logs_object", "logs_msg", "logs_level_display", "logs_period", "logs_since", "logs_until"]
        for key in keys_to_clear:
            if key in st.session_state:
                del st.session_state[key]
        st.rerun()


def make_path(event_type: str, level: str, entity: str, entity_id: str, since: datetime, until: datetime, limit: int, offset: int) -> str:
    parts = []
    if level:
        parts.append(f"level={level}")
    if event_type:
        parts.append(f"event_type={event_type}")
    if entity:
        parts.append(f"entity={entity}")
    if entity_id:
        parts.append(f"entity_id={entity_id}")
    if since:
        parts.append(f"since={since}")
    if until:
        parts.append(f"until={until}")
    parts.append(f"limit={limit}")
    parts.append(f"offset={offset}")
    q = "&".join(parts)
    return f"/api/logs?{q}" if q else "/api/logs"


@st.cache_data(ttl=10)
def fetch_logs(path: str):
    try:
        data = api_get(path) or []
        return data
    except Exception as e:
        st.error(f"Failed to fetch logs: {e}")
        return []


path = make_path(
    event_type,
    level,
    entity,
    entity_id,
    since_dt.strftime("%Y-%m-%d 00:00:00") if since_dt else "",
    until_dt.strftime("%Y-%m-%d 23:59:59") if until_dt else "",
    int(page_size),
    int(page) * int(page_size),
)

data = fetch_logs(path)
df = pd.DataFrame(data)

if message_sub and not df.empty:
    if "message" not in df.columns:
        df["message"] = ""
    df = df[df["message"].astype(str).str.contains(message_sub, case=False, na=False)]

if df.empty:
    st.info(t("no_records"))
else:
    df_display = df.copy()
    
    # Create user-friendly display
    if "ts" in df_display.columns:
        df_display[t("time")] = pd.to_datetime(df_display["ts"]).dt.strftime("%d.%m.%Y %H:%M")
    
    if "level" in df_display.columns:
        level_translation = {
            "INFO": f"ℹ️ {t('information')}",
            "WARN": f"⚠️ {t('warning')}",
            "ERROR": f"❌ {t('error')}",
            "AUDIT": "📋 Audit"
        }
        df_display[t("importance")] = df_display["level"].map(level_translation).fillna(df_display["level"])
    
    if "message" in df_display.columns:
        df_display[t("description")] = df_display["message"]
    
    if "user" in df_display.columns:
        df_display[t("user")] = df_display["user"].fillna(t("system"))
    
    # Select only user-friendly columns
    display_columns = [t("time"), t("importance"), t("description"), t("user")]
    df_display = df_display[[col for col in display_columns if col in df_display.columns]]
    
    total = len(df_display)
    err_count = int((df.get("level", pd.Series(dtype=str)).astype(str) == "ERROR").sum())
    warn_count = int((df.get("level", pd.Series(dtype=str)).astype(str) == "WARN").sum())

    m1, m2, m3 = st.columns(3)
    m1.metric(t("total_records"), total)
    m2.metric(t("errors"), err_count, delta=None if err_count == 0 else f"+{err_count}", delta_color="inverse")
    m3.metric(t("warnings"), warn_count, delta=None if warn_count == 0 else f"+{warn_count}", delta_color="inverse")

    st.subheader(t("event_history"))
    st.dataframe(df_1based(df_display), use_container_width=True, height=400)

    csv = df_display.to_csv(index=False)
    st.download_button(t("download_csv"), data=csv, file_name="logs.csv", mime="text/csv", use_container_width=True)


    # Simple analytics
    st.markdown("---")
    st.subheader(t("analytics"))
    
    c1, c2 = st.columns([1, 1])
    
    with c1:
        st.caption(f"**{t('distribution_by_actions')}**")
        if "event_type" in df.columns:
            # Translate event types
            event_translation = {
                "ship.create": t("ship_create"),
                "ship.update": t("ship_update"),
                "ship.delete": t("ship_delete"),
                "port.create": t("port_create"),
                "port.update": t("port_update"),
                "port.delete": t("port_delete"),
                "company.create": t("company_create"),
                "company.update": t("company_update"),
                "company.delete": t("company_delete"),
                "person.create": t("person_create"),
                "person.update": t("person_update"),
                "person.delete": t("person_delete")
            }
            
            et = df["event_type"].fillna(t("other"))
            et_translated = et.map(event_translation).fillna(et)
            counts = et_translated.value_counts().rename_axis(t("action")).reset_index(name=t("quantity"))
            
            if not counts.empty:
                counts = counts.set_index(t("action"))
                st.bar_chart(counts, height=300)
            else:
                st.info(t("no_data"))
        else:
            st.info(t("no_event_data"))
    
    with c2:
        st.caption(f"**{t('distribution_by_importance')}**")
        if "level" in df.columns:
            level_translation = {
                "INFO": f"ℹ️ {t('information')}",
                "WARN": f"⚠️ {t('warning')}",
                "ERROR": f"❌ {t('error')}",
                "AUDIT": "📋 Audit"
            }
            
            lv = df["level"].fillna(t("other"))
            lv_translated = lv.map(level_translation).fillna(lv)
            counts = lv_translated.value_counts().rename_axis(t("importance")).reset_index(name=t("quantity"))
            
            if not counts.empty:
                counts = counts.set_index(t("importance"))
                st.bar_chart(counts, height=300, color="#ff4444")
            else:
                st.info(t("no_data"))
        else:
            st.info(t("no_level_data"))
    
    # Time series: logs per day
    st.caption(f"**{t('activity_by_days')}**")
    if "ts" in df.columns:
        try:
            df_ts = df.copy()
            df_ts[t("date")] = pd.to_datetime(df_ts["ts"]).dt.date
            ts_counts = df_ts.groupby(t("date")).size().rename(t("events_count"))
            
            if not ts_counts.empty:
                st.line_chart(ts_counts, height=250)
            else:
                st.info(t("no_data"))
        except Exception as e:
            st.caption(f"Error: {e}")
    else:
        st.info(t("no_time_data"))

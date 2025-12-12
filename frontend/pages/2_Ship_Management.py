from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Ships Management", page_icon="🚢", layout="wide")
api.inject_theme()

st.sidebar.title("🚢 Fleet Manager")
st.sidebar.caption("Ships")
from common import get_health
_h = get_health()

col_l, col_c, col_r = st.columns([1, 3, 1])
with col_c:
    st.title("🚢 Ship Management")

if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))


### UI HELPERS
def df_stretch(df: pd.DataFrame):
    try:
        st.dataframe(df, width="stretch")
    except TypeError:
        st.dataframe(df, use_container_width=True)


### LOAD DATA
try:
    ships_df      = api.get_ships()
    ports_df      = api.get_ports()
    companies_df  = api.get_companies()
    types_df      = api.get_ship_types()
except Exception as e:
    st.error(f"Failed to load data from backend: {e}")
    st.stop()

port_map    = api.get_name_map(ports_df) if not ports_df.empty else {}
company_map = api.get_name_map(companies_df) if not companies_df.empty else {}

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

ship_type_map = {}
ship_type_codes = []

if not types_df.empty and "code" in types_df.columns:
    for _, row in types_df.iterrows():
        c = str(row["code"])
        n = str(row.get("name", c))
        ship_type_map[c] = n
    
    ship_type_codes = list(ship_type_map.keys())

def format_ship_type(code: str) -> str:
    return ship_type_map.get(code, code)


### PREPARE OTHER LISTS
port_ids = []
if not ports_df.empty and "id" in ports_df.columns:
    port_ids = ports_df["id"].dropna().astype(int).tolist()

def port_label(pid: int) -> str:
    return port_map.get(pid, f"port id={pid}")

company_ids = [0]
if not companies_df.empty and "id" in companies_df.columns:
    company_ids += companies_df["id"].dropna().astype(int).tolist()

def company_label(cid: int) -> str:
    if cid == 0: return "— (no company)"
    return company_map.get(cid, f"company id={cid}")

SHIP_STATUS_OPTIONS = [
    ("docked",    "⚓ docked — at port"),
    ("loading",   "⬆️ loading — loading cargo"),
    ("unloading", "⬇️ unloading — unloading cargo"),
    ("departed",  "🚢 departed — sailed off"),
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


### STICKY TABS
tab = api.sticky_tabs(
    ["📋 Ship List", "➕ Create Ship", "🛠️ Update", "🚢 Depart Ship", "❌ Delete Ship"],
    "ships_main_tabs",
)

if tab == "📋 Ship List":
    st.subheader("📋 All Ships")

    if ships_df.empty:
        st.info("No ships yet.")
    else:
        departed_ships = ships_df[ships_df["status"] == "departed"] if "status" in ships_df.columns else pd.DataFrame()
        
        if not departed_ships.empty:
            with st.expander("🚢 Ships on Voyage", expanded=True):
                st.markdown("**Active voyages with ETA information:**")
                
                for idx, ship in departed_ships.iterrows():
                    ship_name = ship.get("name", "Unknown")
                    ship_id = ship.get("id", 0)
                    
                    dest_port_id = safe_int(ship.get("destination_port_id", 0))
                    dest_port_name = port_map.get(dest_port_id, f"Port #{dest_port_id}")
                    
                    departed_at = ship.get("departed_at", "")
                    eta = ship.get("eta", "")
                    distance = ship.get("voyage_distance_km", 0)
                    speed = ship.get("speed_knots", 20)
                    
                    from datetime import datetime
                    departed_str = "Unknown"
                    eta_str = "Unknown"
                    
                    try:
                        if departed_at:
                            dep_dt = datetime.fromisoformat(departed_at)
                            departed_str = dep_dt.strftime("%Y-%m-%d %H:%M")
                    except:
                        pass
                    
                    try:
                        if eta:
                            eta_dt = datetime.fromisoformat(eta)
                            eta_str = eta_dt.strftime("%Y-%m-%d %H:%M")
                    except:
                        pass
                    
                    col1, col2, col3, col4 = st.columns([2, 1.5, 1.5, 1])
                    
                    with col1:
                        st.markdown(f"**{ship_name}** (#{ship_id})")
                        st.caption(f"→ Destination: **{dest_port_name}**")
                    
                    with col2:
                        st.caption(f"🚢 Departed:")
                        st.text(departed_str)
                    
                    with col3:
                        st.caption(f"⏱️ ETA:")
                        st.text(eta_str)
                    
                    with col4:
                        st.caption(f"📏 Distance:")
                        st.text(f"{distance:.0f} km")
                    
                    st.markdown("---")
                
                st.info(f"Total ships on voyage: **{len(departed_ships)}**")
        
        view = ships_df.copy()

        if "port_id" in view.columns:
            view["port"] = view["port_id"].map(lambda x: port_map.get(safe_int(x), str(x)))
        if "company_id" in view.columns:
            view["company"] = view["company_id"].map(lambda x: "—" if safe_int(x)==0 else company_map.get(safe_int(x), str(x)))
        if "status" in view.columns:
            view["status"] = view["status"].map(status_fmt)
        if "type" in view.columns:
            view["type"] = view["type"].map(lambda x: ship_type_map.get(x, x))

        final_cols = [c for c in ["id", "name", "type", "country", "status", "port", "company", "speed_knots"] if c in view.columns]

        st.caption(f"Ships found: {len(view)}")
        df_stretch(api.df_1based(view[final_cols]))

        st.markdown("---")
        st.subheader("📊 Fleet Statistics")

        col_mix, col_status = st.columns(2)

        with col_mix:
            if not ships_df.empty and "type" in ships_df.columns:
                try:
                    by_type = ships_df["type"].fillna("(unknown)").map(lambda x: ship_type_map.get(x, x))
                    counts = by_type.value_counts().rename_axis("type").reset_index(name="count").set_index("type")
                    st.markdown("**Structure by Type**")
                    st.bar_chart(counts)
                except Exception:
                    st.caption("No data by types.")

            if not ships_df.empty and "company_id" in ships_df.columns:
                try:
                    comp_map = api.get_name_map(companies_df, id_col="id", name_col="name")
                    comp_series = ships_df["company_id"].fillna(0).astype(int).map(lambda x: comp_map.get(x, "— (no company)"))
                    comp_counts = comp_series.value_counts().rename_axis("company").reset_index(name="count").set_index("company")
                    st.markdown("**Top Companies**")
                    st.bar_chart(comp_counts)
                except Exception:
                    st.caption("No data by companies.")

        with col_status:
            if not ships_df.empty and "status" in ships_df.columns:
                try:
                    status_counts = ships_df["status"].fillna("(unknown)").value_counts().rename_axis("status").reset_index(name="count").set_index("status")
                    st.markdown("**Fleet Status**")
                    st.bar_chart(status_counts)
                except Exception:
                    st.caption("No data by statuses.")

elif tab == "➕ Create Ship":
    st.subheader("➕ Create New Ship")

    if not ship_type_codes:
        st.error("⛔ No ship types in the system!")
        st.info("Please go to **'⚙️ Admin Data' → 'Ship Models Management'** and create at least one type (e.g., 'Passenger').")
    elif not port_ids:
        st.warning("⛔ No ports. First add ports in 'Admin Data'.")
    else:
        with st.form("create_ship_form"):
            name = st.text_input("Ship Name", placeholder="Mriya")
            
            selected_type_code = st.selectbox(
                "Ship Type", 
                ship_type_codes, 
                format_func=format_ship_type
            )
            
            country = st.text_input("Country", value="Ukraine")
            sel_port = st.selectbox("Port", port_ids, format_func=port_label)
            sel_comp = st.selectbox("Company", company_ids, format_func=company_label)
            
            speed_knots = st.number_input(
                "Speed (knots)", 
                min_value=5, 
                max_value=50, 
                value=20, 
                help="Typical speed for different vessel types: 15-20 knots"
            )

            st.caption("ℹ️ Initial status automatically: **docked**")

            if st.form_submit_button("Create"):
                if not name:
                    st.error("Enter name.")
                else:
                    api.api_post("/api/ships", {
                        "name": name,
                        "type": selected_type_code,
                        "country": country,
                        "port_id": int(sel_port),
                        "status": "docked",
                        "company_id": int(sel_comp),
                        "speed_knots": speed_knots
                    }, success_msg=f"Ship '{name}' created.")

elif tab == "🛠️ Update":
    st.subheader("🛠️ Update Data")
    if ships_df.empty:
        st.info("No ships.")
    else:
        ship_ids = ships_df["id"].dropna().astype(int).tolist()
        sid = st.selectbox("Ship", ship_ids, format_func=ship_full_label)
        
        row = ships_df[ships_df["id"] == sid].iloc[0]

        with st.form("upd_ship"):
            new_name = st.text_input("Name", value=str(row.get("name", "")))
            
            cur_code = str(row.get("type", ""))
            idx_type = 0
            if cur_code in ship_type_codes:
                idx_type = ship_type_codes.index(cur_code)
            
            if ship_type_codes:
                new_type = st.selectbox("Type", ship_type_codes, index=idx_type, format_func=format_ship_type)
            else:
                st.warning("Ship types are missing from reference.")
                new_type = st.text_input("Type (code)", value=cur_code, disabled=True)

            new_country = st.text_input("Country", value=str(row.get("country", "")))
            
            cur_pid = safe_int(row.get("port_id", 0))
            pidx = port_ids.index(cur_pid) if cur_pid in port_ids else 0
            new_port = st.selectbox("Port", port_ids, index=pidx, format_func=port_label)

            cur_stat = str(row.get("status", "docked"))
            sidx = STATUS_VALUES.index(cur_stat) if cur_stat in STATUS_VALUES else 0
            new_stat = st.selectbox("Status", STATUS_VALUES, index=sidx, format_func=status_fmt)

            cur_cid = safe_int(row.get("company_id", 0))
            cidx = company_ids.index(cur_cid) if cur_cid in company_ids else 0
            new_comp = st.selectbox("Company", company_ids, index=cidx, format_func=company_label)
            
            cur_speed = float(row.get("speed_knots", 20.0))
            new_speed = st.number_input(
                "Speed (knots)", 
                min_value=5, 
                max_value=50, 
                value=int(cur_speed), 
                help="Typical speed for different vessel types: 15-20 knots"
            )

            if st.form_submit_button("Save"):
                api.api_put(f"/api/ships/{sid}", {
                    "name": new_name,
                    "type": new_type,
                    "country": new_country,
                    "port_id": int(new_port),
                    "status": new_stat,
                    "company_id": int(new_comp),
                    "speed_knots": new_speed
                }, success_msg="Оновлено.")

elif tab == "🚢 Depart Ship":
    st.subheader("🚢 Depart Ship on Voyage")
    
    st.markdown("""
    **Business Rule:** A ship can depart (`departed`) only if:
    - It has an active **captain** in the crew
    - You specify the destination port and departure time
    
    The system automatically calculates ETA based on distance and typical speed.
    """)

    if ships_df.empty:
        st.info("No ships.")
    elif not port_ids:
        st.warning("No ports for destination.")
    else:
        people_df = api.get_people()
        
        def has_captain_on_ship(ship_id: int) -> bool:
            try:
                crew_df = api.get_ship_crew(ship_id)
                
                if not crew_df.empty and not people_df.empty:
                    if "person_id" in crew_df.columns and "id" in people_df.columns:
                        crew_ids = crew_df["person_id"].dropna().astype(int).tolist()
                        crew_people = people_df[people_df["id"].isin(crew_ids)]
                        
                        if "rank" in crew_people.columns:
                            for idx, row in crew_people.iterrows():
                                rank = str(row.get("rank", "")).lower()
                                if "captain" in rank:
                                    return True
                return False
            except:
                return False
        
        available_ships = ships_df.copy()
        if "status" in available_ships.columns:
            available_ships = available_ships[available_ships["status"] != "departed"]
        
        if available_ships.empty:
            st.info("All ships are already on voyage (departed).")
        else:
            ships_with_captain = []
            for idx, row in available_ships.iterrows():
                ship_id = int(row["id"])
                if has_captain_on_ship(ship_id):
                    ships_with_captain.append(ship_id)
            
            if not ships_with_captain:
                st.warning("⚠️ No ships with captain available for departure.")
                st.info("Go to **'Crew & People' → 'Crew Management'** tab and assign captains to ships.")
                
                with st.expander("📋 Ships Without Captain"):
                    for idx, row in available_ships.iterrows():
                        ship_id = int(row["id"])
                        if ship_id not in ships_with_captain:
                            st.caption(f"• {row.get('name', '')} (#{ship_id})")
            else:
                selected_ship_id = st.selectbox(
                    "Select ship for departure",
                    ships_with_captain,
                    format_func=ship_full_label,
                    key="depart_ship_select",
                )
                
                ship_row = ships_df[ships_df["id"] == selected_ship_id].iloc[0]
                ship_name = ship_row.get("name", "")
                current_port_id = safe_int(ship_row.get("port_id", 0))
                current_port = port_map.get(current_port_id, "unknown")
                
                st.markdown(f"**Selected ship:** {ship_name}")
                st.markdown(f"**Current port:** {current_port}")
                st.success("✅ Ship has a captain — ready to depart!")
                
                dest_port = st.selectbox(
                    "Destination Port",
                    port_ids,
                    format_func=port_label,
                    key="depart_dest_port",
                )
                
                st.subheader("⏱️ Voyage Parameters")
                
                col1, col2 = st.columns(2)
                with col1:
                    from datetime import datetime, timedelta
                    
                    manual_time = st.checkbox("✍️ Enter time manually", value=False, key="manual_time_input")
                    
                    if manual_time:
                        datetime_str = st.text_input(
                            "Departure Date and Time",
                            value=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                            help="Format: YYYY-MM-DD HH:MM:SS (e.g.: 2025-12-11 20:30:00)",
                            key="manual_datetime"
                        )
                        try:
                            from datetime import datetime as dt
                            departed_at = dt.strptime(datetime_str, "%Y-%m-%d %H:%M:%S").isoformat()
                            st.success(f"✅ Departure: {datetime_str}")
                        except ValueError:
                            st.error("❌ Invalid date/time format. Use: YYYY-MM-DD HH:MM:SS")
                            st.stop()
                    else:
                        depart_time = st.time_input("Departure Time", value=datetime.now().time(), key="depart_time")
                        depart_date = st.date_input("Departure Date", value=datetime.now().date(), key="depart_date")
                        
                        from datetime import datetime as dt
                        departed_at = dt.combine(depart_date, depart_time).isoformat()
                
                with col2:
                    current_port_data = ports_df[ports_df["id"] == current_port_id]
                    dest_port_data = ports_df[ports_df["id"] == dest_port]
                    
                    voyage_distance = 0
                    if not current_port_data.empty and not dest_port_data.empty:
                        try:
                            lat1 = float(current_port_data.iloc[0].get("lat", 0))
                            lon1 = float(current_port_data.iloc[0].get("lon", 0))
                            lat2 = float(dest_port_data.iloc[0].get("lat", 0))
                            lon2 = float(dest_port_data.iloc[0].get("lon", 0))
                            
                            voyage_distance = api.haversine_distance(lat1, lon1, lat2, lon2)
                            st.info(f"📏 Distance from {current_port} to {port_map.get(dest_port, 'port')}: **{voyage_distance:.1f} km**")
                        except Exception as e:
                            st.warning(f"Failed to calculate distance: {e}")
                            voyage_distance = 500
                    
                    ship_speed_knots = float(ship_row.get("speed_knots", 20.0))
                    
                    st.info(f"⚓ Ship speed: **{ship_speed_knots:.1f} knots** ({ship_speed_knots * 1.852:.1f} km/h)")
                    
                    speed_kmh = ship_speed_knots * 1.852
                    voyage_hours = voyage_distance / speed_kmh if speed_kmh > 0 else 0
                    voyage_days = voyage_hours / 24
                    
                    st.info(f"📊 Travel time: **{voyage_hours:.1f} hours** ({voyage_days:.2f} days)")

                
                from datetime import datetime as dt
                depart_dt = dt.fromisoformat(departed_at)
                eta_dt = depart_dt + timedelta(hours=voyage_hours)
                eta_str = eta_dt.isoformat()
                
                st.info(f"✈️ Expected arrival: **{eta_dt.strftime('%Y-%m-%d %H:%M')}**")
                
                st.warning("⚠️ After departure, status will change to **departed** and ship will leave current port.", icon="⚠️")
                
                if st.button(f"🚢 Depart '{ship_name}' to {port_map.get(dest_port, 'port')}", type="primary"):
                    api.api_put(f"/api/ships/{selected_ship_id}", {
                        "status": "departed",
                        "port_id": int(dest_port),
                        "destination_port_id": int(dest_port),
                        "departed_at": departed_at,
                        "eta": eta_str,
                        "voyage_distance_km": voyage_distance,
                        "speed_knots": ship_speed_knots
                    }, success_msg=f"Ship '{ship_name}' departed on voyage! ETA: {eta_dt.strftime('%Y-%m-%d %H:%M')}")


elif tab == "❌ Delete Ship":
    st.subheader("❌ Delete Ship")
    if ships_df.empty:
        st.info("No ships.")
    else:
        ship_ids = ships_df["id"].dropna().astype(int).tolist()
        del_id = st.selectbox("Select Ship", ship_ids, format_func=ship_full_label, key="del_sel")
        del_name = ships_df[ships_df["id"] == del_id].iloc[0].get("name", "")

        if st.button(f"❌ Delete '{del_name}'", type="primary"):
            api.api_del(f"/api/ships/{del_id}", success_msg="Deleted.")
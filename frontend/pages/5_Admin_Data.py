from __future__ import annotations

import re
import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Admin / Data", page_icon="⚙️", layout="wide")
api.inject_theme()

st.sidebar.title("🚢 Fleet Manager")
st.sidebar.caption("Admin / Data")
from common import get_health
_h = get_health()


col_l, col_c, col_r = st.columns([1, 3, 1])
with col_c:
    st.title("⚙️ Admin Panel and Data")
st.caption("Here we manage Ports and Ship MODELS.")

if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))


### UI HELPERS
def df_stretch(df: pd.DataFrame, **kwargs):
    try:
        st.dataframe(df, width="stretch", **kwargs)
    except TypeError:
        st.dataframe(df, use_container_width=True, **kwargs)


### BASE SHIP TYPES
BASE_TYPES = [
    ("cargo",     "Cargo"),
    ("military",  "Military"),
    ("research",  "Research"),
    ("passenger", "Passenger"),
]
BASE_LABEL = {c: n for c, n in BASE_TYPES}
BASE_CODES = [c for c, _ in BASE_TYPES]


def split_model_code(full_code: str) -> tuple[str, str]:
    if not full_code: return "", ""
    if "_" not in full_code: return "", full_code
    base, rest = full_code.split("_", 1)
    return base, rest


def generate_slug(text: str) -> str:
    s = str(text).lower().strip()
    s = re.sub(r'\s+', '-', s)
    s = re.sub(r'[^a-z0-9\-]', '', s)
    return s


### LOAD
try:
    ports_df = api.get_ports()
    types_df = api.get_ship_types()
    port_map = api.get_name_map(ports_df)
except Exception as e:
    st.error(f"Failed to load references: {e}")
    st.stop()


### MAIN TABS
tab = api.sticky_tabs(
    ["⚓ Port Management", "📋 Ship Models", "📥 Import Real Data", "📤 Export Data"],
    "admin_main_tabs",
)

if tab == "⚓ Port Management":
    st.subheader("Port Management")

    crud = api.sticky_tabs(
        ["📋 List", "➕ Create", "🛠️ Update", "❌ Delete"],
        "admin_ports_crud_tabs",
    )

    if crud == "📋 List":
        st.markdown("### 📋 All Ports")
        
        if not ports_df.empty:
            col1, col2, col3 = st.columns(3)
            with col1:
                st.metric("Total Ports", len(ports_df))
            with col2:
                regions_count = ports_df["region"].nunique() if "region" in ports_df.columns else 0
                st.metric("Regions", regions_count)
            with col3:
                if "region" in ports_df.columns:
                    top_region = ports_df["region"].value_counts().idxmax() if len(ports_df) > 0 else "—"
                    st.metric("Most ports in", top_region)
        
        port_search = st.text_input("🔍 Search by name", placeholder="Type port name...", key="port_filter_search")
        
        filtered_ports = ports_df.copy()
        if port_search and not filtered_ports.empty:
            mask_name = filtered_ports.get("name", pd.Series(dtype=str)).astype(str).str.contains(port_search, case=False, na=False)
            filtered_ports = filtered_ports[mask_name]

        if filtered_ports.empty:
            st.info("No ports found.")
        else:
            st.caption(f"Showing {len(filtered_ports)} port(s)")
            df_stretch(api.df_1based(filtered_ports))

    elif crud == "➕ Create":
        st.markdown("### ➕ Create New Port")
        
        WORLD_REGIONS = [
            "Europe",
            "Asia",
            "Africa",
            "North America",
            "South America",
            "Australia",
            "Antarctica",
            "Arctic",
        ]
        
        with st.form("create_port_form"):
            col1, col2 = st.columns(2)
            
            with col1:
                name = st.text_input("Port Name *", placeholder="e.g., Rotterdam", key="create_port_name")
                region = st.selectbox("Region *", options=WORLD_REGIONS, key="create_port_region")
            
            with col2:
                lat = st.number_input("Latitude *", value=0.0, min_value=-90.0, max_value=90.0, format="%.6f", key="create_port_lat", help="Range: -90 to 90")
                lon = st.number_input("Longitude *", value=0.0, min_value=-180.0, max_value=180.0, format="%.6f", key="create_port_lon", help="Range: -180 to 180")
            
            st.caption("* Required fields")

            if st.form_submit_button("✅ Create Port", type="primary"):
                if name and region:
                    api.api_post(
                        "/api/ports",
                        {"name": name, "region": region, "lat": lat, "lon": lon},
                        success_msg=f"Port '{name}' created in {region}."
                    )
                else:
                    st.error("Port name and region are required.")

    elif crud == "🛠️ Update":
        st.markdown("### 🛠️ Update Port")
        
        if ports_df.empty:
            st.info("No ports to update.")
        else:
            WORLD_REGIONS = [
                "Europe",
                "Asia",
                "Africa",
                "North America",
                "South America",
                "Australia",
                "Antarctica",
                "Arctic",
            ]
            
            port_ids = ports_df["id"].tolist()
            pid = st.selectbox("Select Port to Update", port_ids, format_func=lambda x: port_map.get(x, f"#{x}"))
            row = ports_df[ports_df["id"] == pid].iloc[0]

            with st.form("update_port_form"):
                col1, col2 = st.columns(2)
                
                with col1:
                    new_name = st.text_input("Port Name *", value=str(row.get('name', "")))
                    current_region = str(row.get('region', 'Europe'))
                    region_index = WORLD_REGIONS.index(current_region) if current_region in WORLD_REGIONS else 0
                    new_region = st.selectbox("Region *", options=WORLD_REGIONS, index=region_index)
                
                with col2:
                    new_lat = st.number_input("Latitude *", value=float(row.get('lat', 0.0)), min_value=-90.0, max_value=90.0, format="%.6f")
                    new_lon = st.number_input("Longitude *", value=float(row.get('lon', 0.0)), min_value=-180.0, max_value=180.0, format="%.6f")

                if st.form_submit_button("✅ Update Port", type="primary"):
                    api.api_put(
                        f"/api/ports/{pid}",
                        {"name": new_name, "region": new_region, "lat": new_lat, "lon": new_lon},
                        success_msg=f"Port '{new_name}' updated."
                    )

    elif crud == "❌ Delete":
        st.markdown("### ❌ Delete Port")
        
        if ports_df.empty:
            st.info("No ports to delete.")
        else:
            pid = st.selectbox("Select Port to Delete", ports_df["id"].tolist(), format_func=lambda x: port_map.get(x, f"#{x}"))
            pname = port_map.get(pid, "N/A")

            st.warning("⚠️ Warning: Deleting a port may cause errors if there are ships or companies linked to it!", icon="⚠️")
            
            if st.button(f"❌ Delete '{pname}'", type="primary"):
                api.api_del(f"/api/ports/{pid}", success_msg=f"Port '{pname}' deleted.")


elif tab == "📋 Ship Models":
    st.subheader("Ship Models")
    st.caption("Create models (e.g., 'Panamax', 'Cruiser') based on 4 base categories.")

    crud = api.sticky_tabs(
        ["📋 Model List", "➕ Create Model", "🛠️ Update Model", "❌ Delete Model"],
        "admin_models_crud_tabs",
    )

    if crud == "📋 Model List":
        st.markdown("### 📋 All Ship Models")
        
        if not types_df.empty:
            col1, col2, col3 = st.columns(3)
            with col1:
                st.metric("Total Models", len(types_df))
            with col2:
                base_types_count = len(BASE_CODES)
                st.metric("Base Categories", base_types_count)
            with col3:
                if "code" in types_df.columns:
                    base_counts = {}
                    for code in types_df["code"].astype(str):
                        base = code.split("_")[0] if "_" in code else code
                        base_counts[base] = base_counts.get(base, 0) + 1
                    if base_counts:
                        top_base = max(base_counts, key=base_counts.get)
                        st.metric("Most Models", BASE_LABEL.get(top_base, top_base))
        
        model_search = st.text_input("🔍 Search by name or code", placeholder="Type model name...", key="model_filter_search")

        filtered_types = types_df.copy()
        if model_search and not filtered_types.empty:
            mask_name = filtered_types.get("name", pd.Series(dtype=str)).astype(str).str.contains(model_search, case=False, na=False)
            mask_code = filtered_types.get("code", pd.Series(dtype=str)).astype(str).str.contains(model_search, case=False, na=False)
            filtered_types = filtered_types[mask_name | mask_code]

        if filtered_types.empty:
            st.info("No models found.")
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
            
            st.caption(f"Showing {len(filtered_types)} model(s)")
            df_stretch(api.df_1based(view[final_cols]))

    elif crud == "➕ Create Model":
        with st.form("create_model_form"):
            base_code = st.selectbox(
                "Ship Category (affects crew requirements)",
                options=BASE_CODES,
                format_func=lambda c: BASE_LABEL.get(c, c),
                help="Cargo requires engineer, Military - soldier, etc.",
            )

            model_name = st.text_input(
                "Model Name",
                placeholder="Super Tanker 3000",
                help="Enter descriptive name.",
            )

            auto_code = ""
            if model_name:
                slug = generate_slug(model_name)
                auto_code = f"{base_code}_{slug}"
                st.caption(f"🔒 Technical code will be auto-generated: **`{auto_code}`**")
            else:
                st.caption("🔒 Technical code will be generated after entering name.")

            description = st.text_area("Description (optional)", placeholder="Describe characteristics...")

            if st.form_submit_button("Create Model"):
                if not model_name.strip():
                    st.error("Enter model name.")
                elif not generate_slug(model_name):
                    st.error("Name must contain at least one latin letter or digit.")
                else:
                    api.api_post(
                        "/api/ship-types",
                        {
                            "code": auto_code,
                            "name": model_name.strip(),
                            "description": description,
                        },
                        success_msg=f"Model '{model_name}' created (code: {auto_code}).",
                    )

    elif crud == "🛠️ Update Model":
        if types_df.empty:
            st.info("No models.")
        else:
            def model_label(tid):
                r = types_df[types_df["id"] == tid].iloc[0]
                return f"{r.get('name')} (id={tid})"

            tid = st.selectbox("Select Model", types_df["id"].tolist(), format_func=model_label)
            row = types_df[types_df["id"] == tid].iloc[0]

            with st.form("upd_mod"):
                st.info(f"Editing model: **{row.get('name')}**")
                st.text_input("Technical code (unchangeable)", value=str(row.get('code')), disabled=True)
                
                new_name = st.text_input("Model Name", value=str(row.get('name', '')))
                new_desc = st.text_area("Description", value=str(row.get('description', '')))

                if st.form_submit_button("Save Changes"):
                    if new_name.strip():
                        api.api_put(
                            f"/api/ship-types/{tid}",
                            {
                                "code": str(row.get('code')), # old code
                                "name": new_name.strip(),
                                "description": new_desc
                            },
                            success_msg="Model updated."
                        )
                    else:
                        st.error("Name cannot be empty.")

    elif crud == "❌ Delete Model":
        if types_df.empty:
            st.info("No models.")
        else:
            def model_label2(tid):
                r = types_df[types_df["id"] == tid].iloc[0]
                return f"{r.get('name')} (id={tid})"

            tid = st.selectbox("Model to Delete", types_df["id"].tolist(), format_func=model_label2, key="del_mod")
            row = types_df[types_df["id"] == tid].iloc[0]
            name = str(row.get("name"))

            st.warning("Deleting model will break ships that use it!", icon="⚠️")
            
            if st.button(f"❌ Delete '{name}'", type="primary"):
                api.api_del(f"/api/ship-types/{tid}", success_msg=f"Model '{name}' deleted.")

elif tab == "📥 Import Real Data":
    st.subheader("📥 Import Real Ship and Port Data")
    
    st.markdown("""
    **Available Sources:**
    - 🚢 **Ships:** Dataset from Kaggle/GitHub
    - ⚓ **Ports:** OpenStreetMap Nominatim (free)
    - 🌍 **Coordinates:** automatic geocoding
    """)

    import_tab = api.sticky_tabs(
        ["🚢 Import Ships (CSV)", "⚓ Import Ports (CSV)", "🌍 Geocode Ports"],
        "import_data_tabs",
    )

    if import_tab == "🚢 Import Ships (CSV)":
        st.markdown("### Upload Ships from CSV File")
        
        st.markdown("""
        **CSV Format:** `name,type,country,port_name,company_name`
        
        **Example:**
        ```
        Ever Given,cargo,Egypt,Port Said,Evergreen Marine
        Titanic II,passenger,USA,Miami,White Star Line
        USS Gerald Ford,military,USA,Norfolk,US Navy
        ```
        """)
        
        uploaded_ships = st.file_uploader(
            "Select CSV file with ships",
            type=["csv"],
            key="upload_ships",
        )
        
        if uploaded_ships:
            try:
                ships_import_df = pd.read_csv(uploaded_ships)
                
                st.markdown("**Preview:**")
                st.dataframe(ships_import_df.head(10), use_container_width=True)
                
                required_cols = ["name", "type", "country"]
                missing = [c for c in required_cols if c not in ships_import_df.columns]
                
                if missing:
                    st.error(f"❌ Missing required columns: {', '.join(missing)}")
                else:
                    st.success(f"✅ Found {len(ships_import_df)} ships for import")
                    
                    ports_df_local = api.get_ports()
                    port_name_to_id = {}
                    if not ports_df_local.empty and "name" in ports_df_local.columns:
                        port_name_to_id = dict(zip(ports_df_local["name"], ports_df_local["id"]))
                    
                    companies_df = api.get_companies()
                    company_name_to_id = {}
                    if not companies_df.empty and "name" in companies_df.columns:
                        company_name_to_id = dict(zip(companies_df["name"], companies_df["id"]))
                    
                    default_port = st.selectbox(
                        "Default port (if not specified in CSV)",
                        list(port_name_to_id.keys()) if port_name_to_id else ["No ports"],
                        key="default_port_ships",
                    )
                    
                    if st.button("🚢 Import All Ships", type="primary"):
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
                                
                                port_name = str(row.get("port_name", "")).strip()
                                port_id = port_name_to_id.get(port_name, port_name_to_id.get(default_port, 0))
                                
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
                            status_text.text(f"Imported: {success_count}, errors: {error_count}")
                        
                        st.success(f"✅ Import complete! Success: {success_count}, errors: {error_count}")
                        if success_count > 0:
                            api.clear_all_caches()
                            st.rerun()
                        
            except Exception as e:
                st.error(f"Error reading CSV: {e}")

    elif import_tab == "⚓ Import Ports (CSV)":
        st.markdown("### Upload Ports from CSV File")
        
        st.markdown("""
        **CSV Format:** `name,region,lat,lon`
        
        **Example:**
        ```
        Odesa,Europe,46.4825,30.7233
        Rotterdam,Europe,51.9244,4.4777
        Singapore,Asia,1.2897,103.8501
        New York,North America,40.6895,-74.0447
        ```
        """)
        
        uploaded_ports = st.file_uploader(
            "Select CSV file with ports",
            type=["csv"],
            key="upload_ports",
        )
        
        if uploaded_ports:
            try:
                ports_import_df = pd.read_csv(uploaded_ports)
                
                st.markdown("**Preview:**")
                st.dataframe(ports_import_df.head(10), use_container_width=True)
                
                required_cols = ["name", "region", "lat", "lon"]
                missing = [c for c in required_cols if c not in ports_import_df.columns]
                
                if missing:
                    st.error(f"❌ Missing required columns: {', '.join(missing)}")
                else:
                    st.success(f"✅ Found {len(ports_import_df)} ports for import")
                    
                    if st.button("⚓ Import All Ports", type="primary"):
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
                            status_text.text(f"Imported: {success_count}, errors: {error_count}")
                        
                        st.success(f"✅ Import complete! Success: {success_count}, errors: {error_count}")
                        if success_count > 0:
                            api.clear_all_caches()
                            st.rerun()
                        
            except Exception as e:
                st.error(f"Error reading CSV: {e}")

    elif import_tab == "🌍 Geocode Ports":
        st.markdown("### Auto-fetch Coordinates via OpenStreetMap")
        
        st.markdown("""
        **OpenStreetMap Nominatim API** — free geocoding service.
        
        Enter port names and the system will automatically find coordinates.
        """)
        
        port_names_input = st.text_area(
            "Enter port names (each name on new line)",
            placeholder="Odesa\nRotterdam\nSingapore\nNew York",
            height=150,
        )
        
        default_region = st.text_input("Default region", value="Unknown")
        
        if st.button("🌍 Find Coordinates and Import", type="primary"):
            if not port_names_input.strip():
                st.warning("Enter at least one port name.")
            else:
                import requests
                from time import sleep
                
                port_lines = [line.strip() for line in port_names_input.strip().split("\n") if line.strip()]
                
                st.info(f"Found {len(port_lines)} ports for geocoding...")
                
                success_count = 0
                error_count = 0
                
                progress_bar = st.progress(0)
                status_text = st.empty()
                results_container = st.container()
                
                for idx, port_name in enumerate(port_lines):
                    try:
                        status_text.text(f"Geocoding: {port_name}...")
                        
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
                                st.warning(f"⚠️ {port_name}: not found")
                            error_count += 1
                        
                        sleep(1.1)
                        
                    except Exception as e:
                        with results_container:
                            st.error(f"❌ {port_name}: {e}")
                        error_count += 1
                    
                    progress = (idx + 1) / len(port_lines)
                    progress_bar.progress(progress)
                
                status_text.text("")
                st.success(f"🎉 Geocoding complete! Success: {success_count}, errors: {error_count}")
                
                if success_count > 0:
                    api.clear_all_caches()
                    st.rerun()


elif tab == "📤 Export Data":
    st.subheader("📤 Export Data to CSV")
    
    st.markdown("""
    **Export database to CSV files** for backup or analysis.
    
    You can export:
    - 🚢 Ships (with all details)
    - ⚓ Ports (with coordinates)
    - 👥 People (crew members)
    - 🏢 Companies
    - 👨‍✈️ Crew Assignments
    """)
    
    export_tab = api.sticky_tabs(
        ["🚢 Export Ships", "⚓ Export Ports", "👥 Export People", "🏢 Export Companies", "👨‍✈️ Export Crew"],
        "export_data_tabs",
    )
    
    if export_tab == "🚢 Export Ships":
        st.markdown("### 🚢 Export Ships to CSV")
        
        try:
            ships_df = api.get_ships()
            
            if ships_df.empty:
                st.info("No ships to export.")
            else:
                st.success(f"Found **{len(ships_df)}** ships")
                
                st.markdown("**Preview (first 10 rows):**")
                st.dataframe(ships_df.head(10), use_container_width=True)
                
                csv_data = ships_df.to_csv(index=False)
                
                st.download_button(
                    label="📥 Download Ships CSV",
                    data=csv_data,
                    file_name="ships_export.csv",
                    mime="text/csv",
                    type="primary",
                )
                
        except Exception as e:
            st.error(f"Error exporting ships: {e}")
    
    elif export_tab == "⚓ Export Ports":
        st.markdown("### ⚓ Export Ports to CSV")
        
        try:
            ports_df_export = api.get_ports()
            
            if ports_df_export.empty:
                st.info("No ports to export.")
            else:
                st.success(f"Found **{len(ports_df_export)}** ports")
                
                st.markdown("**Preview:**")
                st.dataframe(ports_df_export, use_container_width=True)
                
                csv_data = ports_df_export.to_csv(index=False)
                
                st.download_button(
                    label="📥 Download Ports CSV",
                    data=csv_data,
                    file_name="ports_export.csv",
                    mime="text/csv",
                    type="primary",
                )
                
        except Exception as e:
            st.error(f"Error exporting ports: {e}")
    
    elif export_tab == "👥 Export People":
        st.markdown("### 👥 Export People to CSV")
        
        try:
            people_df = api.get_people()
            
            if people_df.empty:
                st.info("No people to export.")
            else:
                st.success(f"Found **{len(people_df)}** people")
                
                st.markdown("**Preview (first 10 rows):**")
                st.dataframe(people_df.head(10), use_container_width=True)
                
                csv_data = people_df.to_csv(index=False)
                
                st.download_button(
                    label="📥 Download People CSV",
                    data=csv_data,
                    file_name="people_export.csv",
                    mime="text/csv",
                    type="primary",
                )
                
        except Exception as e:
            st.error(f"Error exporting people: {e}")
    
    elif export_tab == "🏢 Export Companies":
        st.markdown("### 🏢 Export Companies to CSV")
        
        try:
            companies_df = api.get_companies()
            
            if companies_df.empty:
                st.info("No companies to export.")
            else:
                st.success(f"Found **{len(companies_df)}** companies")
                
                st.markdown("**Preview:**")
                st.dataframe(companies_df, use_container_width=True)
                
                csv_data = companies_df.to_csv(index=False)
                
                st.download_button(
                    label="📥 Download Companies CSV",
                    data=csv_data,
                    file_name="companies_export.csv",
                    mime="text/csv",
                    type="primary",
                )
                
        except Exception as e:
            st.error(f"Error exporting companies: {e}")
    

    elif export_tab == "👨‍✈️ Export Crew":
        st.markdown("### 👨‍✈️ Export Crew Assignments to CSV")
        
        st.markdown("""
        **Crew assignments** show which people are assigned to which ships.
        """)
        
        try:
            ships_df = api.get_ships()
            people_df = api.get_people()
            
            if ships_df.empty or people_df.empty:
                st.info("No crew assignments to export.")
            else:
                crew_list = []
                
                for idx, ship in ships_df.iterrows():
                    ship_id = ship.get("id")
                    ship_name = ship.get("name", "")
                    
                    try:
                        crew_df = api.get_ship_crew(ship_id)
                        
                        if not crew_df.empty and "person_id" in crew_df.columns:
                            for idx2, crew_row in crew_df.iterrows():
                                person_id = crew_row.get("person_id")
                                
                                person = people_df[people_df["id"] == person_id]
                                if not person.empty:
                                    person_name = person.iloc[0].get("full_name", "")
                                    person_rank = person.iloc[0].get("rank", "")
                                    
                                    crew_list.append({
                                        "ship_id": ship_id,
                                        "ship_name": ship_name,
                                        "person_id": person_id,
                                        "person_name": person_name,
                                        "rank": person_rank,
                                    })
                    except:
                        pass
                
                if not crew_list:
                    st.info("No active crew assignments found.")
                else:
                    crew_export_df = pd.DataFrame(crew_list)
                    
                    st.success(f"Found **{len(crew_export_df)}** crew assignments")
                    
                    st.markdown("**Preview (first 10 rows):**")
                    st.dataframe(crew_export_df.head(10), use_container_width=True)
                    
                    csv_data = crew_export_df.to_csv(index=False)
                    
                    st.download_button(
                        label="📥 Download Crew Assignments CSV",
                        data=csv_data,
                        file_name="crew_assignments_export.csv",
                        mime="text/csv",
                        type="primary",
                    )
                
        except Exception as e:
            st.error(f"Error exporting crew: {e}")

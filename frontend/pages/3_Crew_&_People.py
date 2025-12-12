from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api
from datetime import datetime, timezone

st.set_page_config(page_title="Crew & People", page_icon="🧑‍✈️", layout="wide")
api.inject_theme()

st.sidebar.title("🚢 Fleet Manager")
st.sidebar.caption("Crew & People")
from common import get_health
_h = get_health()


col_l, col_c, col_r = st.columns([1, 3, 1])
with col_c:
    st.title("🧑‍✈️ Crew and Personnel Management")


### UI HELPERS
def df_stretch(df: pd.DataFrame, **kwargs):
    try:
        st.dataframe(df, width="stretch", **kwargs)
    except TypeError:
        st.dataframe(df, width="stretch", **kwargs)


PROFESSIONS = [
    ("Captain",    "Captain"),
    ("Engineer",   "Engineer"),
    ("Military",   "Military"),
    ("Researcher", "Researcher"),
]

LABEL_BY_CODE = {code: label for code, label in PROFESSIONS}
CODE_BY_LABEL = {label: code for code, label in PROFESSIONS}
PROF_LABELS = [label for _, label in PROFESSIONS]

LEGACY_TO_LABEL = {
    "Солдат": "Military",
    "Soldier": "Military",
    "Капітан": "Captain",
    "Інженер": "Engineer",
    "Дослідник": "Researcher",
    "Військовий": "Military",
}

def rank_to_db(label: str) -> str:
    return label

def rank_to_ui_label(raw_rank: str) -> str:
    if not raw_rank:
        return ""
    if raw_rank in LABEL_BY_CODE:
        return LABEL_BY_CODE[raw_rank]
    if raw_rank in LEGACY_TO_LABEL:
        return LEGACY_TO_LABEL[raw_rank]
    return raw_rank

def default_prof_index_from_db_rank(raw_rank: str) -> int:
    if not raw_rank:
        return 0
    label = LABEL_BY_CODE.get(raw_rank, raw_rank)
    label = LEGACY_TO_LABEL.get(label, label)
    try:
        return PROF_LABELS.index(label)
    except ValueError:
        return 0


if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))


### LOAD
try:
    ships_df  = api.get_ships()
    people_df = api.get_people()
    ship_name_map   = api.get_ship_name_map()
    person_name_map = api.get_person_name_map()
except Exception as e:
    st.error(f"Failed to load references: {e}")
    st.stop()


### STICKY MAIN TABS
tab = api.sticky_tabs(
    ["⚓ Crew Management", "👤 Personnel Management (CRUD)"],
    "crew_people_main_tabs",
)


if tab == "⚓ Crew Management":
    st.subheader("Assign and Remove from Crew")

    if ships_df.empty:
        st.warning("No ships. First create a ship.")
    elif people_df.empty:
        st.warning("No people. First create a person.")
    elif not ship_name_map:
        st.warning("Failed to build ship list for selection.")
    else:
        selected_ship_id = st.selectbox(
            "Select ship for crew management",
            list(ship_name_map.keys()),
            format_func=lambda x: ship_name_map.get(x, "N/A"),
            key="crew_ship_select",
        )
        selected_ship_name = ship_name_map.get(selected_ship_id, "N/A")
        st.markdown(f"**Selected:** {selected_ship_name}")

        col_assign, col_unassign, col_current = st.columns([1, 1, 1.5])

        with col_assign:
            st.markdown("#### ➕ Assign")

            active_person_ids = api.get_all_active_person_ids()

            if "id" in people_df.columns:
                available_people = people_df[~people_df["id"].isin(active_person_ids)]
            else:
                available_people = pd.DataFrame()

            if available_people.empty:
                st.info("No available people to assign.")
            else:
                with st.form("assign_form"):
                    person_options = available_people["id"].tolist()
                    selected_person_id = st.selectbox(
                        "Select available person",
                        person_options,
                        format_func=lambda x: person_name_map.get(x, "N/A"),
                        key="assign_person_select",
                    )
                    submitted = st.form_submit_button("Assign to Crew")

                    if submitted:
                        now_utc = datetime.now(timezone.utc).isoformat()
                        payload = {
                            "person_id": int(selected_person_id),
                            "ship_id": int(selected_ship_id),
                            "start_utc": now_utc,
                        }
                        api.api_post(
                            "/api/crew/assign",
                            payload,
                            success_msg=(
                                f"Person (id={selected_person_id}) assigned to ship."
                            ),
                        )

        with col_unassign:
            st.markdown("#### ➖ Remove")

            crew_df = api.get_ship_crew(selected_ship_id)

            if crew_df.empty or "person_id" not in crew_df.columns:
                st.info("No active crew on this ship.")
            else:
                with st.form("unassign_form"):
                    active_person_options = crew_df["person_id"].tolist()
                    selected_active_person_id = st.selectbox(
                        "Select active crew member",
                        active_person_options,
                        format_func=lambda x: person_name_map.get(x, "N/A"),
                        key="unassign_person_select",
                    )
                    submitted = st.form_submit_button("Remove from Ship", type="primary")

                    if submitted:
                        now_utc = datetime.now(timezone.utc).isoformat()
                        payload = {
                            "person_id": int(selected_active_person_id),
                            "end_utc": now_utc,
                        }
                        api.api_post(
                            "/api/crew/end",
                            payload,
                            success_msg=(
                                f"Person (id={selected_active_person_id}) removed from ship."
                            ),
                        )

        with col_current:
            st.markdown("#### 👥 Current Crew")

            crew_df_current = api.get_ship_crew(selected_ship_id)
            if crew_df_current.empty:
                st.caption("Current crew is empty.")
            else:
                if not people_df.empty and {"id", "full_name", "rank"}.issubset(people_df.columns):
                    people_small = people_df[["id", "full_name", "rank"]].rename(
                        columns={"id": "person_id"}
                    )
                    crew_df_current = crew_df_current.merge(
                        people_small, on="person_id", how="left"
                    )

                if "rank" in crew_df_current.columns:
                    crew_df_current["rank"] = crew_df_current["rank"].map(
                        lambda r: rank_to_ui_label(str(r))
                    )

                display_cols = ["full_name", "rank"]
                crew_display = crew_df_current[[col for col in display_cols if col in crew_df_current.columns]]
                
                df_stretch(
                    api.df_1based(crew_display),
                    height=400,
                )


elif tab == "👤 Personnel Management (CRUD)":
    st.subheader("Personnel List Management")

    people_tab = api.sticky_tabs(
        ["📋 List", "➕ Create", "🛠️ Update", "❌ Delete"],
        "people_crud_tabs",
    )

    if people_tab == "📋 List":
        st.markdown("### 📋 All People")
        
        active_ship_map = api.get_active_ship_map()
        ship_name_map2  = api.get_ship_name_map()

        people_view = people_df.copy()

        if not people_view.empty:
            col1, col2, col3 = st.columns(3)
            with col1:
                st.metric("Total People", len(people_view))
            with col2:
                assigned_count = sum(1 for pid in people_view["id"] if active_ship_map.get(int(pid)))
                st.metric("Assigned to Ships", assigned_count)
            with col3:
                if "rank" in people_view.columns:
                    top_rank = people_view["rank"].value_counts().idxmax() if len(people_view) > 0 else "—"
                    st.metric("Most Common Role", top_rank)
        
        q = st.text_input("🔍 Search by name", placeholder="Type person name...", key="people_search")

        if "rank" in people_view.columns:
            people_view["rank_ui"] = people_view["rank"].map(lambda r: rank_to_ui_label(str(r)))

        if q and "full_name" in people_view.columns:
            people_view = people_view[people_view["full_name"].astype(str).str.contains(q, case=False, na=False)]

        if not people_view.empty and "id" in people_view.columns:
            def current_ship_label(person_id):
                try:
                    pid = int(person_id)
                except Exception:
                    return ""
                ship_id = active_ship_map.get(pid)
                if not ship_id:
                    return ""
                return ship_name_map2.get(ship_id, f"Ship id={ship_id}")

            people_view["current_ship"] = people_view["id"].map(current_ship_label)

            if "rank_ui" in people_view.columns:
                people_view["rank"] = people_view["rank_ui"]

            cols_order = []
            for col in ["id", "full_name", "rank", "active", "current_ship"]:
                if col in people_view.columns:
                    cols_order.append(col)
            for col in people_view.columns:
                if col not in cols_order and col != "rank_ui":
                    cols_order.append(col)

            people_view = people_view[cols_order]

        if people_view.empty:
            st.info("No people found.")
        else:
            st.caption(f"Showing {len(people_view)} person(s)")
            df_stretch(api.df_1based(people_view))

    elif people_tab == "➕ Create":
        with st.form("create_person_form"):
            full_name = st.text_input("Full Name", key="create_person_full_name")

            selected_label = st.selectbox(
                "Profession",
                options=PROF_LABELS,
                key="create_profession_select",
            )

            active = st.checkbox("Active", value=True, key="create_person_active")

            if st.form_submit_button("Create Person"):
                if full_name:
                    api.api_post(
                        "/api/people",
                        {
                            "full_name": full_name,
                            "rank": rank_to_db(selected_label),
                            "active": bool(active),
                        },
                        success_msg=f"Person '{full_name}' created.",
                    )
                else:
                    st.error("Full name is required.")

    elif people_tab == "🛠️ Update":
        if people_df.empty:
            st.info("No people to update.")
        else:
            person_id_to_update = st.selectbox(
                "Select person to update",
                people_df["id"].tolist(),
                format_func=lambda x: person_name_map.get(x, "N/A"),
                key="person_update_select",
            )
            selected_person = people_df[people_df["id"] == person_id_to_update].iloc[0]

            with st.form("update_person_form"):
                new_full_name = st.text_input(
                    "Full Name",
                    value=str(selected_person.get("full_name", "")),
                    key="update_person_full_name",
                )

                current_rank_raw = str(selected_person.get("rank", ""))
                default_index = default_prof_index_from_db_rank(current_rank_raw)

                selected_label = st.selectbox(
                    "Profession",
                    options=PROF_LABELS,
                    index=default_index,
                    key="update_profession_select",
                )

                new_active = st.checkbox(
                    "Active",
                    value=bool(selected_person.get("active", True)),
                    key="update_person_active",
                )

                if st.form_submit_button("Update Data"):
                    if new_full_name:
                        api.api_put(
                            f"/api/people/{person_id_to_update}",
                            {
                                "full_name": new_full_name,
                                "rank": rank_to_db(selected_label),
                                "active": bool(new_active),
                            },
                            success_msg=f"Data for '{new_full_name}' updated.",
                        )
                    else:
                        st.error("Full name is required.")

    elif people_tab == "❌ Delete":
        if people_df.empty:
            st.info("No people to delete.")
        else:
            person_id_to_delete = st.selectbox(
                "Select person to delete",
                people_df["id"].tolist(),
                format_func=lambda x: person_name_map.get(x, "N/A"),
                key="person_delete_select",
            )
            person_name = person_name_map.get(person_id_to_delete, "N/A")

            st.warning("Deleting an active crew member may cause an error.", icon="⚠️")
            if st.button(f"❌ Delete '{person_name}'", type="primary", key="person_delete_btn"):
                api.api_del(
                    f"/api/people/{person_id_to_delete}",
                    success_msg=f"Person '{person_name}' deleted.",
                )

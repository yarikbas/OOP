from __future__ import annotations

import streamlit as st
import pandas as pd
import common as api
from datetime import datetime, timezone

st.set_page_config(page_title="Crew & People", page_icon="🧑‍✈️", layout="wide")
st.title("🧑‍✈️ Управління Екіпажем та Персоналом")

# ============================================================
# Бекенд перевіряє rank українськими рядками.
# UI зберігає в БД саме LABEL.
# ============================================================
PROFESSIONS = [
    ("Captain",    "Капітан"),
    ("Engineer",   "Інженер"),
    ("Soldier",    "Солдат"),
    ("Researcher", "Дослідник"),
]
LABEL_BY_CODE = {code: label for code, label in PROFESSIONS}
CODE_BY_LABEL = {label: code for code, label in PROFESSIONS}
PROF_LABELS = [label for _, label in PROFESSIONS]

def rank_to_db(label: str) -> str:
    return label

def rank_to_ui_label(raw_rank: str) -> str:
    if not raw_rank:
        return ""
    if raw_rank in LABEL_BY_CODE:
        return LABEL_BY_CODE[raw_rank]
    return raw_rank

def default_prof_index_from_db_rank(raw_rank: str) -> int:
    if not raw_rank:
        return 0
    label = LABEL_BY_CODE.get(raw_rank, raw_rank)
    try:
        return PROF_LABELS.index(label)
    except ValueError:
        return 0

# Flash
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

# ================== LOAD ==================
try:
    ships_df  = api.get_ships()
    people_df = api.get_people()
    ship_name_map   = api.get_ship_name_map()
    person_name_map = api.get_person_name_map()
except Exception as e:
    st.error(f"Не вдалося завантажити довідники: {e}")
    st.stop()

# ================== STICKY MAIN TABS ==================
tab = api.sticky_tabs(
    ["⚓ Управління Екіпажами", "👤 Управління Персоналом (CRUD)"],
    "crew_people_main_tabs",
)

# ============================================================
#               УПРАВЛІННЯ ЕКІПАЖАМИ
# ============================================================
if tab == "⚓ Управління Екіпажами":
    st.subheader("Призначення та зняття з екіпажу")

    if ships_df.empty:
        st.warning("Немає кораблів. Спочатку створіть корабель.")
    elif people_df.empty:
        st.warning("Немає людей. Спочатку створіть людину.")
    elif not ship_name_map:
        st.warning("Не вдалося побудувати список кораблів для вибору.")
    else:
        selected_ship_id = st.selectbox(
            "Оберіть корабель для управління екіпажем",
            list(ship_name_map.keys()),
            format_func=lambda x: ship_name_map.get(x, "Н/Д"),
            key="crew_ship_select",
        )
        selected_ship_name = ship_name_map.get(selected_ship_id, "Н/Д")
        st.markdown(f"**Обрано:** {selected_ship_name}")

        col_assign, col_unassign, col_current = st.columns([1, 1, 1.5])

        # ---------- Призначити ----------
        with col_assign:
            st.markdown("#### ➕ Призначити")

            active_person_ids = api.get_all_active_person_ids()

            if "id" in people_df.columns:
                available_people = people_df[~people_df["id"].isin(active_person_ids)]
            else:
                available_people = pd.DataFrame()

            if available_people.empty:
                st.info("Немає вільних людей для призначення.")
            else:
                with st.form("assign_form"):
                    person_options = available_people["id"].tolist()
                    selected_person_id = st.selectbox(
                        "Оберіть вільну людину",
                        person_options,
                        format_func=lambda x: person_name_map.get(x, "Н/Д"),
                        key="assign_person_select",
                    )
                    submitted = st.form_submit_button("Призначити в команду")

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
                                f"Людина (id={selected_person_id}) призначена на корабель."
                            ),
                        )

        # ---------- Зняти ----------
        with col_unassign:
            st.markdown("#### ➖ Зняти")

            crew_df = api.get_ship_crew(selected_ship_id)

            if crew_df.empty or "person_id" not in crew_df.columns:
                st.info("На кораблі немає активного екіпажу.")
            else:
                with st.form("unassign_form"):
                    active_person_options = crew_df["person_id"].tolist()
                    selected_active_person_id = st.selectbox(
                        "Оберіть активного члена екіпажу",
                        active_person_options,
                        format_func=lambda x: person_name_map.get(x, "Н/Д"),
                        key="unassign_person_select",
                    )
                    submitted = st.form_submit_button("Зняти з корабля", type="primary")

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
                                f"Людина (id={selected_active_person_id}) знята з корабля."
                            ),
                        )

        # ---------- Поточний екіпаж ----------
        with col_current:
            st.markdown("#### 👥 Поточний екіпаж")

            crew_df_current = api.get_ship_crew(selected_ship_id)
            if crew_df_current.empty:
                st.caption("Поточний екіпаж порожній.")
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

                st.dataframe(
                    api.df_1based(crew_df_current),
                    use_container_width=True,
                    height=400,
                )

# ============================================================
#           УПРАВЛІННЯ ПЕРСОНАЛОМ (CRUD)
# ============================================================
elif tab == "👤 Управління Персоналом (CRUD)":
    st.subheader("Управління списком персоналу")

    people_tab = api.sticky_tabs(
        ["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"],
        "people_crud_tabs",
    )

    # ---------- Список ----------
    if people_tab == "📋 Список":
        active_ship_map = api.get_active_ship_map()
        ship_name_map2  = api.get_ship_name_map()

        people_view = people_df.copy()

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

            if "rank" in people_view.columns:
                people_view["rank"] = people_view["rank"].map(
                    lambda r: rank_to_ui_label(str(r))
                )

            cols_order = []
            for col in ["id", "full_name", "rank", "active", "current_ship"]:
                if col in people_view.columns:
                    cols_order.append(col)
            for col in people_view.columns:
                if col not in cols_order:
                    cols_order.append(col)

            people_view = people_view[cols_order]

        st.dataframe(api.df_1based(people_view), use_container_width=True)

    # ---------- Створити ----------
    elif people_tab == "➕ Створити":
        with st.form("create_person_form"):
            full_name = st.text_input("Повне ім'я", key="create_person_full_name")

            selected_label = st.selectbox(
                "Професія",
                options=PROF_LABELS,
                key="create_profession_select",
            )

            active = st.checkbox("Активний", value=True, key="create_person_active")

            if st.form_submit_button("Створити людину"):
                if full_name:
                    api.api_post(
                        "/api/people",
                        {
                            "full_name": full_name,
                            "rank": rank_to_db(selected_label),
                            "active": bool(active),
                        },
                        success_msg=f"Людина '{full_name}' створена.",
                    )
                else:
                    st.error("Повне ім'я є обов'язковим.")

    # ---------- Оновити ----------
    elif people_tab == "🛠️ Оновити":
        if people_df.empty:
            st.info("Немає людей для оновлення.")
        else:
            person_id_to_update = st.selectbox(
                "Оберіть людину для оновлення",
                people_df["id"].tolist(),
                format_func=lambda x: person_name_map.get(x, "Н/Д"),
                key="person_update_select",
            )
            selected_person = people_df[people_df["id"] == person_id_to_update].iloc[0]

            with st.form("update_person_form"):
                new_full_name = st.text_input(
                    "Повне ім'я",
                    value=str(selected_person.get("full_name", "")),
                    key="update_person_full_name",
                )

                current_rank_raw = str(selected_person.get("rank", ""))
                default_index = default_prof_index_from_db_rank(current_rank_raw)

                selected_label = st.selectbox(
                    "Професія",
                    options=PROF_LABELS,
                    index=default_index,
                    key="update_profession_select",
                )

                new_active = st.checkbox(
                    "Активний",
                    value=bool(selected_person.get("active", True)),
                    key="update_person_active",
                )

                if st.form_submit_button("Оновити дані"):
                    if new_full_name:
                        api.api_put(
                            f"/api/people/{person_id_to_update}",
                            {
                                "full_name": new_full_name,
                                "rank": rank_to_db(selected_label),
                                "active": bool(new_active),
                            },
                            success_msg=f"Дані '{new_full_name}' оновлено.",
                        )
                    else:
                        st.error("Повне ім'я є обов'язковим.")

    # ---------- Видалити ----------
    elif people_tab == "❌ Видалити":
        if people_df.empty:
            st.info("Немає людей для видалення.")
        else:
            person_id_to_delete = st.selectbox(
                "Оберіть людину для видалення",
                people_df["id"].tolist(),
                format_func=lambda x: person_name_map.get(x, "Н/Д"),
                key="person_delete_select",
            )
            person_name = person_name_map.get(person_id_to_delete, "Н/Д")

            st.warning("Видалення активного члена екіпажу може спричинити помилку.", icon="⚠️")
            if st.button(f"❌ Видалити '{person_name}'", type="primary", key="person_delete_btn"):
                api.api_del(
                    f"/api/people/{person_id_to_delete}",
                    success_msg=f"Людина '{person_name}' видалена.",
                )

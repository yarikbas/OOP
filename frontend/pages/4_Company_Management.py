from __future__ import annotations

import streamlit as st
import pandas as pd
from common import get_health
import common as api

st.set_page_config(page_title="Company Management", page_icon="🏢", layout="wide")
api.inject_theme()

# Sidebar identity and health
st.sidebar.title("🚢 Fleet Manager")
st.sidebar.caption("Company Management")
_h = get_health()

# Center title
col_l, col_c, col_r = st.columns([1, 3, 1])
with col_c:
    st.title("🏢 Управління Компаніями")


# ================== UI HELPERS ==================
def df_stretch(df: pd.DataFrame, **kwargs):
    """
    Сумісне відображення таблиць для нових/старих версій Streamlit.
    Новий API: width="stretch"
    Старий API: use_container_width=True
    """
    try:
        st.dataframe(df, width="stretch", **kwargs)
    except TypeError:
        st.dataframe(df, width="stretch", **kwargs)


# Flash
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))


# ================== LOAD BASE DATA ==================
try:
    companies_df = api.get_companies()
    ports_df     = api.get_ports()
    ships_df     = api.get_ships()
except Exception as e:
    st.error(f"Не вдалося завантажити дані: {e}")
    st.stop()

company_map = api.get_name_map(companies_df) if not companies_df.empty else {}
port_map    = api.get_name_map(ports_df) if not ports_df.empty else {}

def safe_int(x, default=0):
    try:
        if pd.isna(x):
            return default
        return int(x)
    except Exception:
        return default


# ================== STICKY MAIN TABS ==================
tab = api.sticky_tabs(
    ["🏢 Компанії", "⚓ Компанія–Порт", "🚢 Компанія–Кораблі"],
    "company_main_tabs",
)


# =========================================================
# TAB 1: Companies CRUD
# =========================================================
if tab == "🏢 Компанії":
    st.subheader("Список компаній")

    with st.expander("Фільтри", expanded=True):
        f1, f2 = st.columns([2, 1])
        search = f1.text_input("Пошук за назвою", key="company_filter_search")
        sort_by = f2.selectbox(
            "Сортування",
            ["ID ↑", "Назва ↑", "Назва ↓"],
            key="company_filter_sort",
        )
        if st.button("Очистити фільтри", key="company_filter_reset"):
            st.session_state["company_filter_search"] = ""
            st.session_state["company_filter_sort"] = "ID ↑"
            st.rerun()

    filtered = companies_df.copy()
    if not filtered.empty and "name" in filtered.columns and search:
        mask = filtered["name"].astype(str).str.contains(search.strip(), case=False, na=False)
        filtered = filtered[mask]

    if not filtered.empty:
        if sort_by == "Назва ↑" and "name" in filtered.columns:
            filtered = filtered.sort_values(by="name", ascending=True)
        elif sort_by == "Назва ↓" and "name" in filtered.columns:
            filtered = filtered.sort_values(by="name", ascending=False)
        else:
            filtered = filtered.sort_values(by="id", ascending=True, na_position="last")

    col_left, col_right = st.columns([1.1, 1])

    with col_left:
        if filtered.empty:
            st.info("Компаній ще немає.")
        else:
            show_cols = [c for c in ["id", "name"] if c in filtered.columns]
            if not show_cols:
                show_cols = list(filtered.columns)

            df_stretch(api.df_1based(filtered[show_cols]))

    with col_right:
        st.markdown("### ➕ Додати компанію")
        with st.form("company_add_form"):
            new_name = st.text_input("Назва компанії", placeholder="Напр. Oceanic Trade", key="company_create_name")
            submitted = st.form_submit_button("Створити")
            if submitted:
                if not new_name.strip():
                    st.warning("Вкажи назву компанії.")
                else:
                    api.api_post(
                        "/api/companies",
                        {"name": new_name.strip()},
                        success_msg="Компанію створено."
                    )

        st.markdown("---")
        st.markdown("### ✏️ Перейменувати компанію")
        if companies_df.empty or "id" not in companies_df.columns:
            st.caption("Немає компаній для редагування.")
        else:
            ids = [int(x) for x in companies_df["id"].tolist()]
            edit_id = st.selectbox(
                "Компанія",
                ids,
                format_func=lambda x: company_map.get(int(x), str(x)),
                key="company_edit_select",
            )
            edit_name = st.text_input("Нова назва", key="company_edit_name")

            if st.button("Зберегти назву", key="company_edit_btn"):
                if not edit_name.strip():
                    st.warning("Вкажи нову назву.")
                else:
                    api.api_put(
                        f"/api/companies/{int(edit_id)}",
                        {"name": edit_name.strip()},
                        success_msg="Компанію оновлено."
                    )

        st.markdown("---")
        st.markdown("### ❌ Видалити компанію")
        if companies_df.empty or "id" not in companies_df.columns:
            st.caption("Немає компаній для видалення.")
        else:
            del_id = st.selectbox(
                "Компанія для видалення",
                [int(x) for x in companies_df["id"].tolist()],
                format_func=lambda x: company_map.get(int(x), str(x)),
                key="company_delete_select",
            )

            st.warning("Якщо до компанії прив’язані кораблі/порти — можливий 500.", icon="⚠️")
            if st.button("❌ Видалити компанію", type="primary", key="company_delete_btn"):
                api.api_del(
                    f"/api/companies/{int(del_id)}",
                    success_msg="Компанію видалено."
                )


# =========================================================
# TAB 2: Company–Port links
# =========================================================
elif tab == "⚓ Компанія–Порт":
    st.subheader("Управління зв'язками 'Компанія–Порт'")

    if companies_df.empty or ports_df.empty or "id" not in companies_df.columns or "id" not in ports_df.columns:
        st.warning("Для управління зв'язками потрібні хоча б одна компанія та один порт.")
    else:
        company_ids = companies_df["id"].astype(int).tolist()

        selected_company_id = st.selectbox(
            "Оберіть компанію",
            company_ids,
            format_func=lambda x: company_map.get(int(x), "N/A"),
            key="company_port_select",
        )
        selected_company_id = int(selected_company_id)

        st.markdown(f"**Обрана компанія:** {company_map.get(selected_company_id, 'N/A')}")

        current_ports_df = api.get_company_ports(selected_company_id)

        if not current_ports_df.empty:
            if "port_id" not in current_ports_df.columns and "id" in current_ports_df.columns:
                current_ports_df = current_ports_df.rename(columns={"id": "port_id"})

        current_port_ids = set()
        if not current_ports_df.empty and "port_id" in current_ports_df.columns:
            current_ports_df["port_id"] = current_ports_df["port_id"].astype(int)
            current_port_ids = set(current_ports_df["port_id"].tolist())

            with st.expander("Фільтр портів", expanded=True):
                port_filter = st.text_input(
                    "Пошук порту за назвою/регіоном",
                    key="company_port_filter",
                    placeholder="Напр. Odesa або Europe",
                )
                if st.button("Очистити", key="company_port_filter_reset"):
                    st.session_state["company_port_filter"] = ""
                    st.rerun()

            col_add, col_manage = st.columns([1, 1.2])

        # --- Додати порт ---
        with col_add:
            st.markdown("#### ➕ Додати порт")

            available_ports = ports_df.copy()
            available_ports["id"] = available_ports["id"].astype(int)

            available_ports = available_ports[~available_ports["id"].isin(current_port_ids)]

            if port_filter:
                if "name" in available_ports.columns:
                    mask_name = available_ports["name"].astype(str).str.contains(port_filter, case=False, na=False)
                else:
                    mask_name = False
                mask_region = available_ports.get("region", pd.Series(dtype=str)).astype(str).str.contains(port_filter, case=False, na=False)
                available_ports = available_ports[mask_name | mask_region]

            if available_ports.empty:
                st.info("Ця компанія вже присутня у всіх доступних портах.")
            else:
                with st.form("add_port_to_company_form"):
                    port_id_to_add = st.selectbox(
                        "Оберіть порт для додавання",
                        available_ports["id"].tolist(),
                        format_func=lambda x: port_map.get(int(x), "N/A"),
                        key="company_port_add_select",
                    )
                    is_hq = st.checkbox("Це головний порт компанії?", value=False, key="company_port_add_is_hq")

                    if st.form_submit_button("Додати зв'язок"):
                        api.api_post(
                            f"/api/companies/{selected_company_id}/ports",
                            {
                                "port_id": int(port_id_to_add),
                                "is_hq": bool(is_hq),
                            },
                            success_msg="Порт додано до компанії.",
                        )

        # --- Керування ---
        with col_manage:
            st.markdown("#### 📋 Поточні порти компанії")

            if current_ports_df.empty:
                st.info("Ця компанія ще не присутня в жодному порту.")
            else:
                view_df = current_ports_df.copy()
                if "port_id" in view_df.columns:
                    view_df["port_name"] = view_df["port_id"].map(port_map)

                if port_filter:
                    mask_name = view_df.get("port_name", pd.Series(dtype=str)).astype(str).str.contains(port_filter, case=False, na=False)
                    mask_region = view_df.get("region", pd.Series(dtype=str)).astype(str).str.contains(port_filter, case=False, na=False)
                    view_df = view_df[mask_name | mask_region]

                st.caption(
                    "ℹ️ Якщо бекенд ще не повертає прапорець головного порту — "
                    "цей список показує лише прив'язані порти."
                )

                show_cols = [c for c in ["port_id", "port_name"] if c in view_df.columns]
                df_stretch(api.df_1based(view_df[show_cols]))

                st.markdown("#### ⭐ Зробити головним портом")

                with st.form("set_main_port_form"):
                    port_id_to_make_main = st.selectbox(
                        "Оберіть порт зі списку компанії",
                        sorted(list(current_port_ids)),
                        format_func=lambda x: port_map.get(int(x), "N/A"),
                        key="company_port_make_main_select",
                    )
                    if st.form_submit_button("Зробити головним"):
                        api.api_post(
                            f"/api/companies/{selected_company_id}/ports",
                            {
                                "port_id": int(port_id_to_make_main),
                                "is_hq": True,
                            },
                            success_msg="Головний порт оновлено.",
                        )

                st.markdown("#### ❌ Видалити зв'язок")

                port_id_to_delete = st.selectbox(
                    "Оберіть порт для видалення",
                    sorted(list(current_port_ids)),
                    format_func=lambda x: port_map.get(int(x), "N/A"),
                    key="company_port_delete_select",
                )

                if st.button("❌ Видалити зв'язок з цим портом", type="primary", key="company_port_delete_btn"):
                    api.api_del(
                        f"/api/companies/{selected_company_id}/ports/{int(port_id_to_delete)}",
                        success_msg="Порт відв'язано від компанії.",
                    )


# =========================================================
# TAB 3: Company–Ships (view)
# =========================================================
elif tab == "🚢 Компанія–Кораблі":
    st.subheader("Кораблі компанії")

    if companies_df.empty or "id" not in companies_df.columns:
        st.info("Спочатку створи хоча б одну компанію.")
    else:
        company_ids = companies_df["id"].astype(int).tolist()

        selected_company_id = st.selectbox(
            "Оберіть компанію",
            company_ids,
            format_func=lambda x: company_map.get(int(x), "N/A"),
            key="company_ships_select",
        )
        selected_company_id = int(selected_company_id)

        if ships_df.empty or "company_id" not in ships_df.columns:
            st.info("Немає даних про кораблі.")
        else:
            view = ships_df.copy()
            view["company_id"] = view["company_id"].fillna(0).apply(safe_int)

            company_ships = view[view["company_id"] == selected_company_id].copy()

            with st.expander("Фільтри кораблів", expanded=True):
                f1, f2, f3 = st.columns([2, 1, 1])
                ship_search = f1.text_input("Пошук по назві/типу", key="company_ship_filter_search")
                status_options = sorted([s for s in view.get("status", pd.Series(dtype=str)).dropna().unique()]) if "status" in view.columns else []
                type_options = sorted([t for t in view.get("type", pd.Series(dtype=str)).dropna().unique()]) if "type" in view.columns else []
                status_sel = f2.multiselect("Статус", status_options, key="company_ship_filter_status")
                type_sel = f3.multiselect("Тип", type_options, key="company_ship_filter_type")
                if st.button("Очистити фільтри", key="company_ship_filter_reset"):
                    st.session_state["company_ship_filter_search"] = ""
                    st.session_state["company_ship_filter_status"] = []
                    st.session_state["company_ship_filter_type"] = []
                    st.rerun()

            filtered_ships = company_ships.copy()
            if ship_search:
                mask_name = filtered_ships.get("name", pd.Series(dtype=str)).astype(str).str.contains(ship_search, case=False, na=False)
                mask_type = filtered_ships.get("type", pd.Series(dtype=str)).astype(str).str.contains(ship_search, case=False, na=False)
                filtered_ships = filtered_ships[mask_name | mask_type]

            if status_sel and "status" in filtered_ships.columns:
                filtered_ships = filtered_ships[filtered_ships["status"].isin(status_sel)]

            if type_sel and "type" in filtered_ships.columns:
                filtered_ships = filtered_ships[filtered_ships["type"].isin(type_sel)]

            if filtered_ships.empty:
                st.info("У цієї компанії поки немає кораблів.")
            else:
                show_cols = [
                    c for c in ["id", "name", "type", "country", "port_id", "status", "company_id"]
                    if c in filtered_ships.columns
                ]
                df_stretch(api.df_1based(filtered_ships[show_cols]))

    st.caption("💡 Прив’язку корабля до компанії ти вже можеш робити через форму Update на сторінці Ships.")

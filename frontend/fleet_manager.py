import streamlit as st
import pandas as pd
import common as api

# ================== КОНФІГ додатку ==================
st.set_page_config(
    page_title="Fleet Manager Dashboard",
    page_icon="🚢",
    layout="wide",
)

# Якщо з попередньої дії (CRUD / лінки) прийшло повідомлення про успіх
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

# ================== ЗАВАНТАЖЕННЯ ДАНИХ З BACKEND ==================
try:
    health = api.api_get("/health")
    if not (health and health.get("status") == "ok"):
        raise RuntimeError("Backend /health повернув не 'ok'")

    ports_df = api.get_ports()
    ships_df = api.get_ships()
    people_df = api.get_people()
    companies_df = api.get_companies()
    types_df = api.get_ship_types()

except Exception as e:
    st.error(f"💥 Backend недоступний за адресою {api.BASE_URL}")
    st.image("https://http.cat/503", caption="Service Unavailable")
    st.error(f"Деталі помилки: {e}")
    st.stop()

# ================== ПІДГОТОВКА: АКТИВНІ КОРАБЛІ (НЕ departed) ==================
if "status" in ships_df.columns:
    active_ships_df = ships_df[ships_df["status"] != "departed"].copy()
else:
    active_ships_df = ships_df.copy()

# ================== ТІТУЛ + ЗАГАЛЬНА СТАТИСТИКА ==================
st.title("🚢 Fleet Manager Dashboard")
st.markdown("Огляд стану портів, флоту, екіпажу та компаній.")

c1, c2, c3, c4, c5 = st.columns(5)
c1.metric("⚓ Порти", len(ports_df))
c2.metric("📋 Типи кораблів", len(types_df))
c3.metric("🚢 Кораблі (в портах)", len(active_ships_df))
c4.metric("🧑‍✈️ Персонал", len(people_df))
c5.metric("🏢 Компанії", len(companies_df))

st.markdown("---")

# ================== ЯКЩО НЕМАЄ ПОРТІВ – ЗУПИНЯЄМОСЬ ==================
if ports_df.empty:
    st.warning("Немає жодного порту в БД. Додайте порти на сторінці '⚙️ Admin'.")
    st.stop()

# ================== ВИБІР ПОРТУ + ІНФОРМАЦІЯ + ТАБИ ==================
port_names = ports_df["name"].tolist()

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
        help="Кораблі у списку нижче будуть відфільтровані за цим портом.",
    )

    sel_port_row = ports_df[ports_df["name"] == selected_port_name].iloc[0]
    sel_port_id = int(sel_port_row["id"])

    st.caption(
        f"Обраний порт: **{selected_port_name}** "
        f"(id={sel_port_id}, регіон: {sel_port_row['region']})"
    )

    # Кораблі, що зараз *фактично в порту* (НЕ departed)
    if "port_id" in active_ships_df.columns:
        ships_in_port = active_ships_df[
            active_ships_df["port_id"] == sel_port_id
        ].copy()
    else:
        ships_in_port = pd.DataFrame()

    # Компанії, які мають кораблі в цьому порту (серед активних)
    companies_in_port = pd.DataFrame()
    if (
        not ships_in_port.empty
        and "company_id" in ships_in_port.columns
        and not companies_df.empty
        and "id" in companies_df.columns
    ):
        companies_in_port_ids = (
            ships_in_port["company_id"]
            .dropna()
            .astype(int)
            .unique()
            .tolist()
        )
        companies_in_port = companies_df[
            companies_df["id"].isin(companies_in_port_ids)
        ].copy()

    tab_ships, tab_companies, tab_all = st.tabs(
        ["🚢 Кораблі в цьому порту", "🏢 Компанії в порту", "🌍 Всі кораблі"]
    )

    # --- Таб "Кораблі в цьому порту" (без departed) ---
    with tab_ships:
        if ships_in_port.empty:
            st.info("У цьому порту зараз немає кораблів (усі, можливо, відпливли).")
        else:
            view_cols = ["id", "name", "type", "country", "status"]
            if "company_id" in ships_in_port.columns:
                view_cols.append("company_id")

            existing = [c for c in view_cols if c in ships_in_port.columns]

            st.dataframe(
                api.df_1based(
                    ships_in_port[existing]
                ),
                width="stretch",
            )

    # --- Таб "Компанії в порту" ---
    with tab_companies:
        if companies_in_port.empty:
            st.info("У цьому порту зараз немає кораблів жодної компанії.")
        else:
            st.dataframe(
                api.df_1based(
                    companies_in_port[["id", "name"]]
                ),
                width="stretch",
            )

    # --- Таб "Всі кораблі" (включно з departed) ---
    with tab_all:
        all_view_cols = ["id", "name", "type", "country", "status"]
        if "port_id" in ships_df.columns:
            all_view_cols.append("port_id")
        if "company_id" in ships_df.columns:
            all_view_cols.append("company_id")

        existing_all = [c for c in all_view_cols if c in ships_df.columns]

        st.dataframe(
            api.df_1based(
                ships_df[existing_all]
            ),
            width="stretch",
        )

# ================== КАРТА ПОРТІВ ==================
with col_map:
    st.subheader("Карта портів")

    ports_for_map = ports_df.rename(columns={"lat": "latitude", "lon": "longitude"})

    if not {"latitude", "longitude"}.issubset(ports_for_map.columns):
        st.error("У таблиці портів немає координат lat/lon.")
    else:
        st.map(
            ports_for_map[["latitude", "longitude"]],
            width="stretch",
        )

st.markdown("---")
st.caption(
    "Для CRUD-управління портами, кораблями, компаніями, екіпажем та зв'язками "
    "скористайтесь сторінками в бічному меню."
)

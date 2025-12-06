import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Company Management", page_icon="🏢", layout="wide")
st.title("🏢 Управління Компаніями")

# Показати останнє повідомлення про успіх, якщо є
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))

# ================== ЗАВАНТАЖЕННЯ ДАНИХ ==================
try:
    companies_df = api.get_companies()
    ports_df = api.get_ports()
except Exception as e:
    st.error(f"Не вдалося завантажити довідники: {e}")
    st.stop()

# Очікувана форма даних з бекенда:
# /api/companies -> [{ "id": int, "name": str }]
# /api/ports     -> [{ "id": int, "name": str, ... }]
if not companies_df.empty and "id" not in companies_df.columns:
    st.error("Очікується, що /api/companies повертає поле 'id'. Зараз його немає. Перевір CompaniesController.")
    st.stop()

if not ports_df.empty and "id" not in ports_df.columns:
    st.error("Очікується, що /api/ports повертає поле 'id'. Зараз його немає. Перевір PortsController.")
    st.stop()

# Мапи id -> name (якщо датафрейми порожні — мапи просто будуть порожні)
company_map = api.get_name_map(companies_df) if not companies_df.empty else {}
port_map = api.get_name_map(ports_df) if not ports_df.empty else {}

# ================== ТАБИ ==================
tab_crud, tab_ports = st.tabs([
    "🏢 Управління Компаніями (CRUD)",
    "🔗 Зв'язки Компанія-Порт"
])

# ---------- CRUD КОМПАНІЙ ----------
with tab_crud:
    st.subheader("Список, створення, оновлення, видалення компаній")

    crud_tabs = st.tabs(["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"])

    # --- Список ---
    with crud_tabs[0]:
        if companies_df.empty:
            st.info("Поки що немає жодної компанії.")
        else:
            st.dataframe(api.df_1based(companies_df), use_container_width=True)

    # --- Створити ---
    with crud_tabs[1]:
        with st.form("create_company_form"):
            name = st.text_input("Назва нової компанії", placeholder="Maersk")

            if st.form_submit_button("Створити компанію"):
                if name:
                    api.api_post(
                        "/api/companies",
                        {"name": name},
                        success_msg=f"Компанія '{name}' створена."
                        # rerun за замовчуванням = True → таблиця оновиться
                    )
                else:
                    st.error("Назва є обов'язковою.")

    # --- Оновити ---
    with crud_tabs[2]:
        if companies_df.empty:
            st.info("Немає компаній для оновлення.")
        else:
            company_ids = companies_df["id"].tolist()
            company_id_to_update = st.selectbox(
                "Оберіть компанію для оновлення",
                company_ids,
                format_func=lambda x: company_map.get(x, "N/A"),
                key="company_update_select",
            )
            selected_company = companies_df[companies_df["id"] == company_id_to_update].iloc[0]

            with st.form("update_company_form"):
                st.write(f"Оновлення: {selected_company['name']}")
                new_name = st.text_input("Нова назва", value=selected_company["name"])

                if st.form_submit_button("Оновити назву"):
                    if new_name:
                        api.api_put(
                            f"/api/companies/{company_id_to_update}",
                            {"name": new_name},
                            success_msg=f"Назву компанії оновлено на '{new_name}'."
                        )
                    else:
                        st.error("Назва є обов'язковою.")

    # --- Видалити ---
    with crud_tabs[3]:
        if companies_df.empty:
            st.info("Немає компаній для видалення.")
        else:
            company_ids = companies_df["id"].tolist()
            company_id_to_delete = st.selectbox(
                "Оберіть компанію для видалення",
                company_ids,
                format_func=lambda x: company_map.get(x, "N/A"),
                key="company_delete_select",
            )
            company_name = company_map.get(company_id_to_delete, "N/A")

            st.warning(
                "Видалення компанії *не* видалить її кораблі, але вони втратять зв'язок.",
                icon="⚠️",
            )
            if st.button(f"❌ Видалити '{company_name}'", type="primary"):
                api.api_del(
                    f"/api/companies/{company_id_to_delete}",
                    success_msg=f"Компанія '{company_name}' видалена."
                )

# ---------- ЗВ'ЯЗКИ КОМПАНІЯ-ПОРТ ----------
with tab_ports:
    st.subheader("Управління зв'язками 'Компанія-Порт'")

    if companies_df.empty or ports_df.empty:
        st.warning("Для управління зв'язками потрібні хоча б одна компанія та один порт.")
    else:
        # --- Вибір компанії ---
        company_ids = companies_df["id"].tolist()
        selected_company_id = st.selectbox(
            "Оберіть компанію",
            company_ids,
            format_func=lambda x: company_map.get(x, "N/A"),
            key="company_port_select",
        )
        st.markdown(f"**Обрана компанія:** {company_map.get(selected_company_id, 'N/A')}")

        col_add, col_list = st.columns(2)

        # --- Отримання поточних портів компанії ---
        current_company_ports_df = api.get_company_ports(selected_company_id)

        # Нормалізуємо назви колонок, щоб у фронті завжди були 'port_id' та 'is_main'
        if current_company_ports_df.empty:
            current_port_ids = set()
        else:
            df = current_company_ports_df.copy()

            # 1) port_id: якщо бекенд повертає 'id' як порт, перейменовуємо
            if "port_id" not in df.columns:
                if "id" in df.columns:
                    df = df.rename(columns={"id": "port_id"})
                else:
                    st.error("Очікується, що /api/companies/{id}/ports повертає 'port_id' або 'id'.")
                    st.dataframe(df)
                    st.stop()

            # 2) прапорець головного порту:
            if "is_main" not in df.columns:
                if "main" in df.columns:
                    df = df.rename(columns={"main": "is_main"})
                else:
                    df["is_main"] = False

            df["port_id"] = df["port_id"].astype(int)

            current_company_ports_df = df
            current_port_ids = set(current_company_ports_df["port_id"].tolist())

        # --- Колонка 1: Додати зв'язок ---
        with col_add:
            st.markdown("#### ➕ Додати порт")

            available_ports = ports_df[~ports_df["id"].isin(current_port_ids)]

            if available_ports.empty:
                st.info("Ця компанія вже присутня у всіх доступних портах.")
            else:
                with st.form("add_port_to_company_form"):
                    port_id_to_add = st.selectbox(
                        "Оберіть порт для додавання",
                        available_ports["id"].tolist(),
                        format_func=lambda x: port_map.get(x, "N/A"),
                    )
                    is_main = st.checkbox("Це головний порт компанії?", value=False)

                    if st.form_submit_button("Додати зв'язок"):
                        api.api_post(
                            f"/api/companies/{selected_company_id}/ports",
                            {
                                "port_id": int(port_id_to_add),
                                "main": bool(is_main),  # ключ 'main' як на бекенді
                            },
                            success_msg=f"Порт (id={port_id_to_add}) додано до компанії.",
                        )

        # --- Колонка 2: Список/Видалення ---
        with col_list:
            st.markdown("#### 📋 Поточні порти компанії")

            if current_company_ports_df.empty:
                st.info("Ця компанія ще не присутня в жодному порту.")
            else:
                # Додаємо назву порту
                current_company_ports_df["port_name"] = current_company_ports_df["port_id"].map(port_map)
                st.dataframe(
                    api.df_1based(current_company_ports_df[["port_id", "port_name", "is_main"]]),
                    use_container_width=True,
                )

                port_id_to_delete = st.selectbox(
                    "Оберіть порт для видалення",
                    current_company_ports_df["port_id"].tolist(),
                    format_func=lambda x: port_map.get(x, "N/A"),
                    key="company_port_delete_select",
                )

                if st.button("❌ Видалити зв'язок з цим портом", type="primary"):
                    api.api_del(
                        f"/api/companies/{selected_company_id}/ports/{port_id_to_delete}",
                        success_msg=f"Порт (id={port_id_to_delete}) відв'язано від компанії.",
                    )

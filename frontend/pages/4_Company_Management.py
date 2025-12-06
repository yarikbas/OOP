# ---------- ЗВ'ЯЗКИ КОМПАНІЯ-ПОРТ ----------
with tab_ports:
    st.subheader("Управління зв'язками 'Компанія-Порт'")

    if companies_df.empty or ports_df.empty:
        st.warning("Для управління зв'язками потрібні хоча б одна компанія та один порт.")
    else:
        company_ids = companies_df["id"].tolist()
        selected_company_id = st.selectbox(
            "Оберіть компанію",
            company_ids,
            format_func=lambda x: company_map.get(x, "N/A"),
            key="company_port_select",
        )
        st.markdown(f"**Обрана компанія:** {company_map.get(selected_company_id, 'N/A')}")

        # Поточні порти компанії (бекенд повертає список портів без прапорця main)
        current_ports_df = api.get_company_ports(selected_company_id)

        # Нормалізація: очікуємо, що це список портів
        if not current_ports_df.empty:
            if "port_id" not in current_ports_df.columns and "id" in current_ports_df.columns:
                current_ports_df = current_ports_df.rename(columns={"id": "port_id"})

        current_port_ids = set()
        if not current_ports_df.empty and "port_id" in current_ports_df.columns:
            current_ports_df["port_id"] = current_ports_df["port_id"].astype(int)
            current_port_ids = set(current_ports_df["port_id"].tolist())

        col_add, col_manage = st.columns([1, 1.2])

        # --- Колонка 1: Додати новий порт ---
        with col_add:
            st.markdown("#### ➕ Додати порт")

            available_ports = ports_df.copy()
            if "id" in available_ports.columns:
                available_ports = available_ports[~available_ports["id"].astype(int).isin(current_port_ids)]

            if available_ports.empty:
                st.info("Ця компанія вже присутня у всіх доступних портах.")
            else:
                with st.form("add_port_to_company_form"):
                    port_id_to_add = st.selectbox(
                        "Оберіть порт для додавання",
                        available_ports["id"].astype(int).tolist(),
                        format_func=lambda x: port_map.get(x, "N/A"),
                    )
                    is_hq = st.checkbox("Це головний порт компанії?", value=False)

                    if st.form_submit_button("Додати зв'язок"):
                        api.api_post(
                            f"/api/companies/{selected_company_id}/ports",
                            {
                                "port_id": int(port_id_to_add),
                                "is_hq": bool(is_hq),  # ✅ сумісно з CompaniesController
                            },
                            success_msg="Порт додано до компанії.",
                        )

        # --- Колонка 2: Перегляд + керування існуючими ---
        with col_manage:
            st.markdown("#### 📋 Поточні порти компанії")

            if current_ports_df.empty:
                st.info("Ця компанія ще не присутня в жодному порту.")
            else:
                # Додаємо красиві назви
                view_df = current_ports_df.copy()
                view_df["port_name"] = view_df["port_id"].map(port_map)

                # Поки що не показуємо true/false main,
                # бо бекенд не повертає цей прапорець
                st.caption("ℹ️ Бекенд поки не повертає, який порт є головним.")

                st.dataframe(
                    api.df_1based(view_df[["port_id", "port_name"]]),
                    use_container_width=True,
                )

                st.markdown("#### ⭐ Зробити головним портом")

                with st.form("set_main_port_form"):
                    port_id_to_make_main = st.selectbox(
                        "Оберіть порт зі списку компанії",
                        sorted(list(current_port_ids)),
                        format_func=lambda x: port_map.get(x, "N/A"),
                    )
                    if st.form_submit_button("Зробити головним"):
                        # Той самий endpoint, той самий метод repo.addPort()
                        api.api_post(
                            f"/api/companies/{selected_company_id}/ports",
                            {
                                "port_id": int(port_id_to_make_main),
                                "is_hq": True,  # ✅ встановлюємо головний
                            },
                            success_msg="Головний порт оновлено.",
                        )

                st.markdown("#### ❌ Видалити зв'язок")

                port_id_to_delete = st.selectbox(
                    "Оберіть порт для видалення",
                    sorted(list(current_port_ids)),
                    format_func=lambda x: port_map.get(x, "N/A"),
                    key="company_port_delete_select",
                )

                if st.button("❌ Видалити зв'язок з цим портом", type="primary"):
                    api.api_del(
                        f"/api/companies/{selected_company_id}/ports/{port_id_to_delete}",
                        success_msg="Порт відв'язано від компанії.",
                    )

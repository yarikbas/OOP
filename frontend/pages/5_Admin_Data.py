from __future__ import annotations

import re
import streamlit as st
import pandas as pd
import common as api

st.set_page_config(page_title="Admin Data", page_icon="⚙️", layout="wide")
st.title("⚙️ Адміністрування Довідників")
st.caption("Тут керуємо Портами та МОДЕЛЯМИ кораблів.")

# Flash
if "last_success" in st.session_state:
    st.success(st.session_state.pop("last_success"))


# ================== UI HELPERS ==================
def df_stretch(df: pd.DataFrame, **kwargs):
    try:
        st.dataframe(df, width="stretch", **kwargs)
    except TypeError:
        st.dataframe(df, use_container_width=True, **kwargs)


# ================== BASE SHIP TYPES ==================
# Це жорстко зашиті категорії, які розуміє C++ бекенд
BASE_TYPES = [
    ("cargo",     "Вантажний"),
    ("military",  "Військовий"),
    ("research",  "Дослідницький"),
    ("passenger", "Пасажирський"),
]
BASE_LABEL = {c: n for c, n in BASE_TYPES}
BASE_CODES = [c for c, _ in BASE_TYPES]


def split_model_code(full_code: str) -> tuple[str, str]:
    """Розбиває code='cargo_panamax' на ('cargo', 'panamax')"""
    if not full_code: return "", ""
    if "_" not in full_code: return "", full_code
    base, rest = full_code.split("_", 1)
    return base, rest


def generate_slug(text: str) -> str:
    """
    Генерує чистий хвостик коду з назви:
    "Super Tanker 3000" -> "super-tanker-3000"
    """
    s = str(text).lower().strip()
    # Замінюємо пробіли на дефіси
    s = re.sub(r'\s+', '-', s)
    # Залишаємо тільки латиницю, цифри і дефіс
    # (Кирилицю можна було б транслітерувати, але для простоти просто чистимо)
    s = re.sub(r'[^a-z0-9\-]', '', s)
    return s


# ================== LOAD ==================
try:
    ports_df = api.get_ports()
    types_df = api.get_ship_types()
    port_map = api.get_name_map(ports_df)
except Exception as e:
    st.error(f"Не вдалося завантажити довідники: {e}")
    st.stop()


# ================== MAIN TABS ==================
tab = api.sticky_tabs(
    ["⚓ Управління Портами", "📋 Моделі Кораблів"],
    "admin_main_tabs",
)

# -------------------------------------------------------------------
#                               PORTS
# -------------------------------------------------------------------
if tab == "⚓ Управління Портами":
    st.subheader("Управління Портами")

    crud = api.sticky_tabs(
        ["📋 Список", "➕ Створити", "🛠️ Оновити", "❌ Видалити"],
        "admin_ports_crud_tabs",
    )

    # Список
    if crud == "📋 Список":
        if ports_df.empty:
            st.info("Портів ще немає.")
        else:
            df_stretch(api.df_1based(ports_df))

    # Створити
    elif crud == "➕ Створити":
        with st.form("create_port_form"):
            name = st.text_input("Назва порту", placeholder="Odesa", key="create_port_name")
            region = st.text_input("Регіон", placeholder="Europe", key="create_port_region")
            lat = st.number_input("Широта", value=46.48, format="%.6f", key="create_port_lat")
            lon = st.number_input("Довгота", value=30.72, format="%.6f", key="create_port_lon")

            if st.form_submit_button("Створити порт"):
                if name and region:
                    api.api_post(
                        "/api/ports",
                        {"name": name, "region": region, "lat": lat, "lon": lon},
                        success_msg=f"Порт '{name}' створено."
                    )
                else:
                    st.error("Назва та Регіон є обов'язковими.")

    # Оновити
    elif crud == "🛠️ Оновити":
        if ports_df.empty:
            st.info("Немає портів для оновлення.")
        else:
            port_ids = ports_df["id"].tolist()
            pid = st.selectbox("Оберіть порт", port_ids, format_func=lambda x: port_map.get(x, "N/A"))
            row = ports_df[ports_df["id"] == pid].iloc[0]

            with st.form("update_port_form"):
                new_name = st.text_input("Назва", value=str(row.get('name', "")))
                new_region = st.text_input("Регіон", value=str(row.get('region', "")))
                new_lat = st.number_input("Широта", value=float(row.get('lat', 0.0)), format="%.6f")
                new_lon = st.number_input("Довгота", value=float(row.get('lon', 0.0)), format="%.6f")

                if st.form_submit_button("Оновити порт"):
                    api.api_put(
                        f"/api/ports/{pid}",
                        {"name": new_name, "region": new_region, "lat": new_lat, "lon": new_lon},
                        success_msg=f"Порт '{new_name}' оновлено."
                    )

    # Видалити
    elif crud == "❌ Видалити":
        if ports_df.empty:
            st.info("Немає портів для видалення.")
        else:
            pid = st.selectbox("Порт для видалення", ports_df["id"].tolist(), format_func=lambda x: port_map.get(x, "N/A"))
            pname = port_map.get(pid, "N/A")

            st.warning("Видалення порту призведе до помилки, якщо там є кораблі!", icon="⚠️")
            if st.button(f"❌ Видалити '{pname}'", type="primary"):
                api.api_del(f"/api/ports/{pid}", success_msg=f"Порт '{pname}' видалено.")


# -------------------------------------------------------------------
#                           SHIP MODELS
# -------------------------------------------------------------------
elif tab == "📋 Моделі Кораблів":
    st.subheader("Моделі кораблів")
    st.caption("Створюйте моделі (наприклад 'Panamax', 'Cruiser') на основі 4-х базових категорій.")

    crud = api.sticky_tabs(
        ["📋 Список моделей", "➕ Створити модель", "🛠️ Оновити модель", "❌ Видалити модель"],
        "admin_models_crud_tabs",
    )

    # --------- LIST ---------
    if crud == "📋 Список моделей":
        if types_df.empty:
            st.info("Моделей ще немає.")
        else:
            view = types_df.copy()
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
            df_stretch(api.df_1based(view[final_cols]))

    # --------- CREATE MODEL ---------
    elif crud == "➕ Створити модель":
        with st.form("create_model_form"):
            # 1. Вибір категорії (це впливає на бізнес-логіку)
            base_code = st.selectbox(
                "Категорія корабля (впливає на вимоги до екіпажу)",
                options=BASE_CODES,
                format_func=lambda c: BASE_LABEL.get(c, c),
                help="Вантажний потребує інженера, Військовий - солдата тощо.",
            )

            # 2. Назва моделі
            model_name = st.text_input(
                "Назва моделі",
                placeholder="Super Tanker 3000",
                help="Введіть зрозумілу назву.",
            )

            # 3. Автогенерація коду (візуалізація)
            auto_code = ""
            if model_name:
                slug = generate_slug(model_name)
                auto_code = f"{base_code}_{slug}"
                st.caption(f"🔒 Технічний код буде згенеровано автоматично: **`{auto_code}`**")
            else:
                st.caption("🔒 Технічний код буде згенеровано після введення назви.")

            description = st.text_area("Опис (опційно)", placeholder="Опис характеристик...")

            if st.form_submit_button("Створити модель"):
                if not model_name.strip():
                    st.error("Введіть назву моделі.")
                elif not generate_slug(model_name):
                    st.error("Назва повинна містити хоча б одну латинську літеру або цифру.")
                else:
                    api.api_post(
                        "/api/ship-types",
                        {
                            "code": auto_code,
                            "name": model_name.strip(),
                            "description": description,
                        },
                        success_msg=f"Модель '{model_name}' створено (код: {auto_code}).",
                    )

    # --------- UPDATE MODEL ---------
    elif crud == "🛠️ Оновити модель":
        if types_df.empty:
            st.info("Немає моделей.")
        else:
            def model_label(tid):
                r = types_df[types_df["id"] == tid].iloc[0]
                return f"{r.get('name')} (id={tid})"

            tid = st.selectbox("Оберіть модель", types_df["id"].tolist(), format_func=model_label)
            row = types_df[types_df["id"] == tid].iloc[0]

            with st.form("upd_mod"):
                st.info(f"Редагування моделі: **{row.get('name')}**")
                # Код міняти не даємо, бо це зламає існуючі кораблі
                st.text_input("Технічний код (незмінний)", value=str(row.get('code')), disabled=True)
                
                new_name = st.text_input("Назва моделі", value=str(row.get('name', '')))
                new_desc = st.text_area("Опис", value=str(row.get('description', '')))

                if st.form_submit_button("Зберегти зміни"):
                    if new_name.strip():
                        api.api_put(
                            f"/api/ship-types/{tid}",
                            {
                                "code": str(row.get('code')), # старий код
                                "name": new_name.strip(),
                                "description": new_desc
                            },
                            success_msg="Модель оновлено."
                        )
                    else:
                        st.error("Назва не може бути порожньою.")

    # --------- DELETE MODEL ---------
    elif crud == "❌ Видалити модель":
        if types_df.empty:
            st.info("Немає моделей.")
        else:
            def model_label2(tid):
                r = types_df[types_df["id"] == tid].iloc[0]
                return f"{r.get('name')} (id={tid})"

            tid = st.selectbox("Модель для видалення", types_df["id"].tolist(), format_func=model_label2, key="del_mod")
            row = types_df[types_df["id"] == tid].iloc[0]
            name = str(row.get("name"))

            st.warning("Видалення моделі зламає кораблі, які її використовують!", icon="⚠️")
            
            if st.button(f"❌ Видалити '{name}'", type="primary"):
                api.api_del(f"/api/ship-types/{tid}", success_msg=f"Модель '{name}' видалено.")
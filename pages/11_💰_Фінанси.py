import streamlit as st
import requests
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from io import BytesIO

st.set_page_config(page_title="Фінансові Звіти", page_icon="💰", layout="wide")

lang = st.session_state.get('lang', 'uk')
t = {
    'uk': {
        'title': '💰 Фінансові Звіти',
        'voyage_expenses': 'Витрати на подорожі',
        'add_expense': 'Додати витрати',
        'voyage': 'Подорож',
        'fuel_cost': 'Вартість палива ($)',
        'port_fees': 'Портові збори ($)',
        'crew_wages': 'Зарплата екіпажу ($)',
        'maintenance': 'Обслуговування ($)',
        'other_costs': 'Інші витрати ($)',
        'total_cost': 'Загальні витрати ($)',
        'submit': 'Зберегти',
        'expense_breakdown': 'Розподіл витрат',
        'cost_trends': 'Тренди витрат',
        'profitability': 'Прибутковість за кораблями',
        'export_excel': 'Експорт в Excel',
        'total_expenses': 'Всього витрат',
        'avg_expense': 'Середні витрати',
        'expense_by_category': 'Витрати по категоріях',
    },
    'en': {
        'title': '💰 Financial Reports',
        'voyage_expenses': 'Voyage Expenses',
        'add_expense': 'Add Expense',
        'voyage': 'Voyage',
        'fuel_cost': 'Fuel Cost ($)',
        'port_fees': 'Port Fees ($)',
        'crew_wages': 'Crew Wages ($)',
        'maintenance': 'Maintenance ($)',
        'other_costs': 'Other Costs ($)',
        'total_cost': 'Total Cost ($)',
        'submit': 'Submit',
        'expense_breakdown': 'Expense Breakdown',
        'cost_trends': 'Cost Trends',
        'profitability': 'Profitability by Ship',
        'export_excel': 'Export to Excel',
        'total_expenses': 'Total Expenses',
        'avg_expense': 'Average Expense',
        'expense_by_category': 'Expenses by Category',
    }
}[lang]

API_URL = "http://localhost:8082"

st.title(t['title'])

# Add new expense
with st.expander(t['add_expense'], expanded=False):
    with st.form("add_expense"):
        # Get voyages
        voyages_response = requests.get(f"{API_URL}/api/voyages")
        voyages = voyages_response.json().get('value', []) if voyages_response.status_code == 200 else []
        
        # Get ships and ports for display
        ships_response = requests.get(f"{API_URL}/api/ships")
        ships = ships_response.json().get('value', []) if ships_response.status_code == 200 else []
        ships_dict = {s['id']: s['name'] for s in ships}
        
        ports_response = requests.get(f"{API_URL}/api/ports")
        ports = ports_response.json().get('value', []) if ports_response.status_code == 200 else []
        ports_dict = {p['id']: p['name'] for p in ports}
        
        voyage_options = {}
        for v in voyages:
            ship_name = ships_dict.get(v['ship_id'], 'Unknown')
            origin = ports_dict.get(v['origin_port_id'], 'Unknown')
            dest = ports_dict.get(v['destination_port_id'], 'Unknown')
            voyage_options[f"{ship_name}: {origin} → {dest} ({v['departed_at'][:10]})"] = v['id']
        
        voyage = st.selectbox(t['voyage'], options=list(voyage_options.keys()))
        
        col1, col2 = st.columns(2)
        
        with col1:
            fuel_cost = st.number_input(t['fuel_cost'], min_value=0.0, step=100.0)
            port_fees = st.number_input(t['port_fees'], min_value=0.0, step=50.0)
            crew_wages = st.number_input(t['crew_wages'], min_value=0.0, step=100.0)
        
        with col2:
            maintenance = st.number_input(t['maintenance'], min_value=0.0, step=50.0)
            other_costs = st.number_input(t['other_costs'], min_value=0.0, step=50.0)
        
        total = fuel_cost + port_fees + crew_wages + maintenance + other_costs
        st.metric(t['total_cost'], f"${total:,.2f}")
        
        submitted = st.form_submit_button(t['submit'])
        if submitted:
            expense_data = {
                'voyage_record_id': voyage_options[voyage],
                'fuel_cost_usd': fuel_cost,
                'port_fees_usd': port_fees,
                'crew_wages_usd': crew_wages,
                'maintenance_cost_usd': maintenance,
                'other_costs_usd': other_costs,
                'total_cost_usd': total
            }
            response = requests.post(f"{API_URL}/api/voyages/expenses", json=expense_data)
            if response.status_code in [200, 201]:
                st.success('Витрати успішно додано!' if lang == 'uk' else 'Expense successfully added!')
                st.rerun()
            else:
                st.error(f"Помилка: {response.text}" if lang == 'uk' else f"Error: {response.text}")

# Get all expenses
expenses_response = requests.get(f"{API_URL}/api/voyages/expenses")

if expenses_response.status_code == 200:
    expenses = expenses_response.json().get('value', [])
    
    if expenses:
        df = pd.DataFrame(expenses)
        
        # Summary metrics
        col1, col2, col3 = st.columns(3)
        col1.metric(t['total_expenses'], f"${df['total_cost_usd'].sum():,.0f}")
        col2.metric(t['avg_expense'], f"${df['total_cost_usd'].mean():,.0f}")
        col3.metric('Записів' if lang == 'uk' else 'Records', len(expenses))
        
        # Expense breakdown pie chart
        st.subheader(t['expense_breakdown'])
        
        categories = {
            'Паливо' if lang == 'uk' else 'Fuel': df['fuel_cost_usd'].sum(),
            'Портові збори' if lang == 'uk' else 'Port Fees': df['port_fees_usd'].sum(),
            'Зарплата' if lang == 'uk' else 'Wages': df['crew_wages_usd'].sum(),
            'Обслуговування' if lang == 'uk' else 'Maintenance': df['maintenance_cost_usd'].sum(),
            'Інше' if lang == 'uk' else 'Other': df['other_costs_usd'].sum(),
        }
        
        breakdown_df = pd.DataFrame(list(categories.items()), columns=['Category', 'Cost'])
        fig = px.pie(breakdown_df, values='Cost', names='Category', hole=0.4)
        st.plotly_chart(fig, use_container_width=True)
        
        # Cost trends over time
        st.subheader(t['cost_trends'])
        
        # Merge with voyages to get dates
        voyages_df = pd.DataFrame(voyages)
        if not voyages_df.empty:
            merged = df.merge(voyages_df[['id', 'ship_id', 'departed_at']], 
                             left_on='voyage_record_id', right_on='id', how='left')
            merged['ship_name'] = merged['ship_id'].map(ships_dict)
            
            if 'departed_at' in merged.columns:
                fig = px.line(merged, x='departed_at', y='total_cost_usd', color='ship_name',
                             labels={'departed_at': 'Дата' if lang == 'uk' else 'Date',
                                    'total_cost_usd': 'Витрати ($)' if lang == 'uk' else 'Cost ($)',
                                    'ship_name': 'Корабель' if lang == 'uk' else 'Ship'})
                st.plotly_chart(fig, use_container_width=True)
            
            # Profitability by ship
            st.subheader(t['profitability'])
            
            voyages_with_profit = voyages_df.copy()
            voyages_with_profit['profit'] = voyages_with_profit['total_revenue_usd'] - voyages_with_profit['total_cost_usd']
            voyages_with_profit['ship_name'] = voyages_with_profit['ship_id'].map(ships_dict)
            
            profit_by_ship = voyages_with_profit.groupby('ship_name')['profit'].sum().reset_index()
            profit_by_ship = profit_by_ship.sort_values('profit', ascending=True)
            
            fig = px.bar(profit_by_ship, y='ship_name', x='profit', orientation='h',
                        labels={'ship_name': 'Корабель' if lang == 'uk' else 'Ship',
                               'profit': 'Прибуток ($)' if lang == 'uk' else 'Profit ($)'},
                        color='profit',
                        color_continuous_scale=['red', 'yellow', 'green'])
            st.plotly_chart(fig, use_container_width=True)
        
        # Export to Excel
        if st.button(t['export_excel']):
            output = BytesIO()
            
            with pd.ExcelWriter(output, engine='openpyxl') as writer:
                df.to_excel(writer, sheet_name='Expenses', index=False)
                breakdown_df.to_excel(writer, sheet_name='Breakdown', index=False)
                if not voyages_df.empty:
                    voyages_with_profit.to_excel(writer, sheet_name='Profitability', index=False)
            
            output.seek(0)
            
            st.download_button(
                label='📥 Завантажити Excel' if lang == 'uk' else '📥 Download Excel',
                data=output,
                file_name=f'financial_report_{pd.Timestamp.now().strftime("%Y%m%d")}.xlsx',
                mime='application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'
            )
        
        # Detailed table
        st.subheader('Детальна таблиця' if lang == 'uk' else 'Detailed Table')
        st.dataframe(df, use_container_width=True)
        
    else:
        st.info('Немає даних про витрати' if lang == 'uk' else 'No expense data found')
else:
    st.error(f"Помилка: {expenses_response.text}" if lang == 'uk' else f"Error: {expenses_response.text}")

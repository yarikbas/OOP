import sqlite3

db_path = r'C:\Users\user\OneDrive\Desktop\OOP\backend\build\Release\data\app.db'

try:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # Get all tables
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table'")
    tables = cursor.fetchall()
    
    print(f"Database: {db_path}")
    print(f"Total tables: {len(tables)}")
    print("\nTables:")
    for table in tables:
        print(f"  - {table[0]}")
        
    conn.close()
except Exception as e:
    print(f"Error: {e}")

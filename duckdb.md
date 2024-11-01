# DuckDB

- [DuckDB](#duckdb)
  - [C++](#c)
  - [Python](#python)
    - [duckdb stream mode](#duckdb-stream-mode)

## C++

You better build DuckDB from [source](https://github.com/duckdb/duckdb) with C++20 as `*.dll` or `*.so`, then linked to your project.

## Python

insert polars dataframe to duckdb

```py
import polars as pl
import duckdb

df=pl.read_ipc('20231*.ipc')

conn=duckdb.connect("bar1d.db", read_only=False)
conn.execute("CREATE TABLE etf AS SELECT * FROM df")
conn.close()
```

append `pl.Dataframe` to an existed table in duckdb

```py
import polars as pl
import duckdb

conn = duckdb.connect("bar1d.db", read_only=False)
df_new = pl.read_ipc("2024*.ipc")
conn.execute("INSERT INTO etf SELECT * FROM df_new")
```

query **datetime** in duckdb

```py
import duckdb

conn=duckdb.connect('bar1d.db', read_only=True)

# pl() to polars dataframe
conn.execute("SELECT * FROM etf WHERE dt>'2023-12-01 00:00:00.000' AND code=1").pl()

```

extract with a **datetime range**

```py
query = """
SELECT *
FROM etf
WHERE dt BETWEEN '2023-11-01' AND '2023-11-10'
"""
conn.execute(query).pl()
```

extract with datetime

```py
query = """
SELECT *
FROM etf
WHERE EXTRACT(MONTH FROM dt) = 11 AND EXTRACT(YEAR FROM dt) = 2023
"""
conn.execute(query).pl()
```

**dynamic filter** with variable

```py
cutoff = dt.datetime(2023, 12, 10, 0, 0, 0)

# Execute a parameterized query
query = """
SELECT *
FROM etf
WHERE dt > ?
"""

conn.execute(query, [cutoff]).pl()
```

### duckdb stream mode

```py
import duckdb
import datetime as dt

# Initialize DuckDB in-memory database
conn = duckdb.connect()

# Create a table to store streaming data
conn.execute("""
CREATE TABLE stream_data (
    timestamp TIMESTAMP,
    value DOUBLE
)
""")


def insert_data(conn, timestamp, value):
    # Insert new data point into the table
    conn.execute("INSERT INTO stream_data VALUES (?, ?)", (timestamp, value))


def calculate_sma(conn):
    # Calculate SMA using a window function
    query = """
    SELECT sma5,sma10,sma20 FROM (
        SELECT 
            timestamp, 
            value, 
            AVG(value) OVER (ORDER BY timestamp ROWS BETWEEN 4 PRECEDING AND CURRENT ROW) AS sma5,
            AVG(value) OVER (ORDER BY timestamp ROWS BETWEEN 9 PRECEDING AND CURRENT ROW) AS sma10,
            AVG(value) OVER (ORDER BY timestamp ROWS BETWEEN 19 PRECEDING AND CURRENT ROW) AS sma20,
        FROM 
            stream_data
    )
    ORDER BY 
        timestamp DESC
    LIMIT 1
    """
    # # below is less efficient
    # query = """
    # SELECT
    #     timestamp,
    #     value,
    #     AVG(value) OVER (ORDER BY timestamp ROWS BETWEEN 4 PRECEDING AND CURRENT ROW) AS sma5,
    #     AVG(value) OVER (ORDER BY timestamp ROWS BETWEEN 9 PRECEDING AND CURRENT ROW) AS sma10,
    #     AVG(value) OVER (ORDER BY timestamp ROWS BETWEEN 19 PRECEDING AND CURRENT ROW) AS sma20,
    # FROM
    #     stream_data
    # ORDER BY
    #     timestamp DESC
    # LIMIT
    #     1
    # """
    return conn.execute(query).fetchone()


for i in range(1, 10000):  # Simulate real-time points
    now = dt.datetime.fromtimestamp(1730300000 + i)
    insert_data(conn, now, i)

    # Calculate and display SMA
    value = calculate_sma(conn)
    print(f"Current SMA at {i}: {value}")
```
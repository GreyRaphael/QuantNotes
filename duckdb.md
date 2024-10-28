# DuckDB

- [DuckDB](#duckdb)
  - [C++](#c)
  - [Python](#python)

## C++

You better build DuckDB from [source](https://github.com/duckdb/duckdb) with C++20 as `*.dll` or `*.so`, then linked to your project.

## Python

insert polars dataframe to duckdb

```py
import polars as pl
import duckdb

df=pl.read_ipc('20231*.ipc')

conn=duckdb.connect("etf_db", read_only=False)
conn.execute("CREATE TABLE kl1m_2023 AS SELECT * FROM df")
conn.close()
```

query **datetime** in duckdb

```py
import duckdb

conn=duckdb.connect('etf_db', read_only=True)

# pl() to polars dataframe
conn.execute("SELECT * FROM kl1m_2023 WHERE dt>'2023-12-01 00:00:00.000' and code=1").pl()

```

extract with a **datetime range**

```py
query = """
SELECT *
FROM kl1m_2023
WHERE dt BETWEEN '2023-11-01' AND '2023-11-10'
"""
conn.execute(query).pl()
```

extract with datetime

```py
query = """
SELECT *
FROM kl1m_2023
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
FROM etf_kl1m_2023
WHERE dt > ?
"""

conn.execute(query, [cutoff]).pl()
```
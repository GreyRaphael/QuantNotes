# DuckDB

- [DuckDB](#duckdb)
  - [C++](#c)
  - [Python](#python)
    - [duckdb stream mode](#duckdb-stream-mode)
    - [duckdb add new column](#duckdb-add-new-column)
  - [Rust](#rust)

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
conn.close() # must close after save
```

append `pl.Dataframe` to an existed table in duckdb

```py
import polars as pl
import duckdb

conn = duckdb.connect("bar1d.db", read_only=False)
df_new = pl.read_ipc("2024*.ipc")
conn.execute("INSERT INTO etf SELECT * FROM df_new")
conn.close() # must close after save
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

### duckdb add new column

```py
import duckdb
import polars as pl

conn=duckdb.connect('bar1d.db', read_only=False)
# add new column
conn.execute("ALTER TABLE etf ADD COLUMN sector UINT64")

# read data
df=pl.read_ipc('sector.ipc')
conn.execute('UPDATE etf SET sector=df.sector FROM df WHERE etf.code=df.code')
conn.close()
```

## Rust

simplest way `cargo add duckdb --features bundled`

```rs
use duckdb::{params, Connection, Result};

#[derive(Debug)]
struct Stock {
    code: u32,
    dt: i32,
    close: u32,
}

fn main() -> Result<()> {
    let path = "bar1d.db";
    let conn = Connection::open(&path)?;

    let mut stmt = conn.prepare("SELECT code, dt, close*adjfactor FROM etf WHERE code = ?")?;
    let code = 510050;
    let rows = stmt.query_map(params![code], |row| {
        Ok((
            row.get::<_, u32>(0)?, // code
            row.get::<_, i32>(1)?, // date
            row.get::<_, f64>(2)?, // close
        ))
    })?;

    for row in rows {
        let (code, date, close) = row?;
        println!("Code: {}, Date: {}, Close: {}", code, date, close);
    }

    Ok(())
}
```

arrow interface: query results without requiring predefined structures

```rs
use duckdb::{Connection, Result};
use duckdb::arrow::record_batch::RecordBatch;
use duckdb::arrow::util::pretty::print_batches;
use duckdb::arrow::array::ArrayRef;

fn main() -> Result<()> {
    let path = "bar1d.db";
    let conn = Connection::open(&path)?;

    // Define your SQL query
    let sql = "SELECT code, dt, close*adjfactor FROM etf WHERE code = 510050";

    // Prepare and execute the query
    let mut stmt = conn.prepare(sql)?;
    let arrow = stmt.query_arrow([])?;

    // Collect the Record Batches
    let record_batches: Vec<RecordBatch> = arrow.collect();
    // Print the Record Batches for inspection
    print_batches(&record_batches);

    // Iterate over each Record Batch
    for batch in &record_batches {
        // Get the schema to access column names
        let schema = batch.schema();
        let columns = schema.fields();

        // Iterate over each column
        for (i, column) in columns.iter().enumerate() {
            let column_name = column.name();
            let array: &ArrayRef = batch.column(i);

            // Process the array as needed
            // For example, print the column name and number of rows
            println!("Column: {}, Rows: {}", column_name, array.len());
        }
    }

    Ok(())
}
```

multithread query duckdb for CPU-bound tasks
> `cargo add rayon`  
> `cargo add duckdb --features bundled`  

```rs
use duckdb::{params, Connection, Result};
use rayon::prelude::*; // Import rayon for parallel iteration

struct DuckdbReplayer {
    path: String,
}

impl DuckdbReplayer {
    fn new(path: &str) -> Self {
        Self {
            path: path.to_string(),
        }
    }

    // Method to query the stock data based on the code
    fn query_stock_data(&self, code: u32) -> Result<Vec<(u32, i32, f64)>> {
        let conn = Connection::open(&self.path)?;
        let mut stmt = conn.prepare("SELECT code, dt, close*adjfactor FROM etf WHERE code = ?")?;
        let rows = stmt.query_map(params![code], |row| {
            Ok((
                row.get::<_, u32>(0)?, // code
                row.get::<_, i32>(1)?, // date
                row.get::<_, f64>(2)?, // close
            ))
        })?;

        // Collect the results into a vector
        let mut result = Vec::with_capacity(1024);
        for row in rows {
            result.push(row?);
        }

        Ok(result)
    }

    // Method to query multiple stock codes in parallel using Rayon
    fn query_multiple_stock_data(&self, codes: &[u32]) -> Vec<Result<Vec<(u32, i32, f64)>>> {
        // Use Rayon to process the codes in parallel
        codes
            .into_par_iter() // parallel iterator
            .map(|&code| self.query_stock_data(code)) // Map over the codes and perform queries
            .collect() // Collect all the results into a vector
    }
}

fn main() {
    let replayer = DuckdbReplayer::new("bar1d.db");
    let codes = vec![510050, 513500, 159659];
    let stock_data = replayer.query_multiple_stock_data(&codes);

    // Print the results
    for (i, data) in stock_data.iter().enumerate() {
        println!("Results for code {}:", codes[i]);
        match data {
            Ok(code_records) => {
                for (code, date, close) in code_records {
                    println!("  Code: {}, Date: {}, Close: {}", code, date, close);
                }
            }
            Err(e) => {
                println!("  Error querying code {}: {}", codes[i], e);
            }
        }
    }
}
```
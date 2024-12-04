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

duckdb backtest generic engine

```rs
use bktrader::datatype::bar::Bar;
use duckdb::{params, Connection};
use std::marker::PhantomData;

// Define the Tick struct
#[derive(Debug)]
struct Tick {
    code: u32,
    open: f64,
    high: f64,
    low: f64,
    close: f64,
}

// Define Strategies
struct StrategyA;
struct StrategyB;

// Trait to handle different data types
trait Strategy<T> {
    fn on_quote(&mut self, quote: &T);
}

// Implement Strategy for StrategyA and StrategyB for Bar
impl Strategy<Bar> for StrategyA {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyA bar: {:?}", quote);
    }
}

impl Strategy<Bar> for StrategyB {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyB bar: {:?}", quote);
    }
}

// Implement Strategy for StrategyA and StrategyB for Tick
impl Strategy<Tick> for StrategyA {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyA tick: {:?}", quote);
    }
}

impl Strategy<Tick> for StrategyB {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyB tick: {:?}", quote);
    }
}

// Trait to map database rows to structs
trait FromRow {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error>
    where
        Self: Sized;
}

// Implement FromRow for Bar
impl FromRow for Bar {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Bar {
            code: row.get(0)?,
            dt: row.get(1)?,
            preclose: row.get(2)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
            netvalue: row.get(7)?,
            volume: row.get(8)?,
            amount: row.get(9)?,
            trades_count: row.get(10)?,
            turnover: row.get(11)?,
        })
    }
}

// Implement FromRow for Tick
impl FromRow for Tick {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Tick {
            code: row.get(0)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
        })
    }
}

// Generic Engine struct
// PhantomData: To indicate that the Engine struct is generic over T even though it doesn't directly use it.
struct Engine<T, S>
where
    S: Strategy<T>,
{
    uri: String,
    strategy: S,
    code: u32,
    start: String,
    end: String,
    _marker: PhantomData<T>,
}

impl<T, S> Engine<T, S>
where
    S: Strategy<T>,
    T: FromRow,
{
    fn new(uri: &str, strategy: S, code: u32, start: &str, end: &str) -> Self {
        Self {
            uri: uri.into(),
            strategy,
            code,
            start: start.into(),
            end: end.into(),
            _marker: PhantomData,
        }
    }

    fn run(&mut self) -> Result<(), duckdb::Error> {
        let conn = Connection::open(&self.uri)?;
        let query = r#"
            SELECT
                code,
                date_diff('day', DATE '1970-01-01', dt) AS days_since_epoch,
                ROUND(preclose * adjfactor / 1e4, 3) AS adj_preclose,
                ROUND(open * adjfactor / 1e4, 3) AS adj_open,
                ROUND(high * adjfactor / 1e4, 3) AS adj_high,
                ROUND(low * adjfactor / 1e4, 3) AS adj_low,
                ROUND(close * adjfactor / 1e4, 3) AS adj_close,
                ROUND(netvalue * adjfactor / 1e4, 3) AS adj_netvalue,
                volume,
                ROUND(amount * adjfactor / 1e4, 3) AS adj_amount,
                COALESCE(trades_count, 0) AS trades_count,
                turnover
            FROM
                etf
            WHERE
                preclose IS NOT NULL
                AND code = ?
                AND dt BETWEEN CAST(? AS DATE) AND CAST(? AS DATE)
        "#;

        let mut stmt = conn.prepare(&query)?;
        let rows = stmt.query_map(params![self.code, self.start, self.end], |row| T::from_row(row))?;

        for row in rows {
            let data = row?;
            self.strategy.on_quote(&data);
        }

        Ok(())
    }
}

fn main() -> Result<(), duckdb::Error> {
    // Define the database URI
    let uri = "bar1d.db";
    // Define the date range
    let start_date = "2024-11-20";
    let end_date = "2024-12-31";

    // List of codes to process
    let code_list: Vec<u32> = vec![510050, 513500, 159659];

    // Process Bars with different strategies
    for &code in &code_list {
        // StrategyA for Bars
        let strategy_a = StrategyA;
        let mut engine_a = Engine::<Bar, _>::new(uri, strategy_a, code, start_date, end_date);
        engine_a.run()?;

        // StrategyB for Bars
        let strategy_b = StrategyB;
        let mut engine_b = Engine::<Bar, _>::new(uri, strategy_b, code, start_date, end_date);
        engine_b.run()?;
    }

    // Process Ticks with different strategies
    for &code in &code_list {
        // StrategyA for Ticks
        let strategy_a = StrategyA;
        let mut engine_a = Engine::<Tick, _>::new(uri, strategy_a, code, start_date, end_date);
        engine_a.run()?;

        // StrategyB for Ticks
        let strategy_b = StrategyB;
        let mut engine_b = Engine::<Tick, _>::new(uri, strategy_b, code, start_date, end_date);
        engine_b.run()?;
    }

    Ok(())
}
```

duckdb backtest with only one Genrics

```rs
use bktrader::datatype::bar::Bar;
use duckdb::{params, Connection};
use std::marker::PhantomData;

// Define the Tick struct
#[derive(Debug)]
struct Tick {
    code: u32,
    open: f64,
    high: f64,
    low: f64,
    close: f64,
}

// Define Strategies
struct StrategyA;
struct StrategyB;

// Trait to handle different data types
trait Strategy<T> {
    fn on_quote(&mut self, quote: &T);
}

// Implement Strategy for StrategyA and StrategyB for Bar
impl Strategy<Bar> for StrategyA {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyA bar: {:?}", quote);
    }
}

impl Strategy<Bar> for StrategyB {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyB bar: {:?}", quote);
    }
}

// Implement Strategy for StrategyA and StrategyB for Tick
impl Strategy<Tick> for StrategyA {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyA tick: {:?}", quote);
    }
}

impl Strategy<Tick> for StrategyB {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyB tick: {:?}", quote);
    }
}

// Trait to map database rows to structs
trait FromRow {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error>
    where
        Self: Sized;
}

// Implement FromRow for Bar
impl FromRow for Bar {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Bar {
            code: row.get(0)?,
            dt: row.get(1)?,
            preclose: row.get(2)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
            netvalue: row.get(7)?,
            volume: row.get(8)?,
            amount: row.get(9)?,
            trades_count: row.get(10)?,
            turnover: row.get(11)?,
        })
    }
}

// Implement FromRow for Tick
impl FromRow for Tick {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Tick {
            code: row.get(0)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
        })
    }
}

enum StrategyType {
    A(StrategyA),
    B(StrategyB),
}

impl Strategy<Bar> for StrategyType {
    fn on_quote(&mut self, quote: &Bar) {
        match self {
            StrategyType::A(s) => s.on_quote(quote),
            StrategyType::B(s) => s.on_quote(quote),
        }
    }
}

impl Strategy<Tick> for StrategyType {
    fn on_quote(&mut self, quote: &Tick) {
        match self {
            StrategyType::A(s) => s.on_quote(quote),
            StrategyType::B(s) => s.on_quote(quote),
        }
    }
}

// Generic Engine struct
// PhantomData: To indicate that the Engine struct is generic over T even though it doesn't directly use it.
struct Engine<T> {
    uri: String,
    strategies: Vec<StrategyType>,
    code: u32,
    start: String,
    end: String,
    _marker: PhantomData<T>,
}

impl<T> Engine<T>
where
    T: FromRow,
    StrategyType: Strategy<T>,
{
    fn new(uri: &str, code: u32, start: &str, end: &str) -> Self {
        Self {
            uri: uri.into(),
            strategies: Vec::with_capacity(8),
            code,
            start: start.into(),
            end: end.into(),
            _marker: PhantomData,
        }
    }

    fn add_strategy(&mut self, stg: StrategyType) {
        self.strategies.push(stg);
    }

    fn run(&mut self) -> Result<(), duckdb::Error> {
        let conn = Connection::open(&self.uri)?;
        let query = r#"
            SELECT
                code,
                date_diff('day', DATE '1970-01-01', dt) AS days_since_epoch,
                ROUND(preclose * adjfactor / 1e4, 3) AS adj_preclose,
                ROUND(open * adjfactor / 1e4, 3) AS adj_open,
                ROUND(high * adjfactor / 1e4, 3) AS adj_high,
                ROUND(low * adjfactor / 1e4, 3) AS adj_low,
                ROUND(close * adjfactor / 1e4, 3) AS adj_close,
                ROUND(netvalue * adjfactor / 1e4, 3) AS adj_netvalue,
                volume,
                ROUND(amount * adjfactor / 1e4, 3) AS adj_amount,
                COALESCE(trades_count, 0) AS trades_count,
                turnover
            FROM
                etf
            WHERE
                preclose IS NOT NULL
                AND code = ?
                AND dt BETWEEN CAST(? AS DATE) AND CAST(? AS DATE)
        "#;

        let mut stmt = conn.prepare(&query)?;
        let rows = stmt.query_map(params![self.code, self.start, self.end], |row| T::from_row(row))?;

        for row in rows {
            let data = row?;
            for stg in self.strategies.iter_mut() {
                stg.on_quote(&data);
            }
        }

        Ok(())
    }
}

fn main() -> Result<(), duckdb::Error> {
    // Define the database URI
    let uri = "bar1d.db";
    // Define the date range
    let start_date = "2024-11-20";
    let end_date = "2024-12-31";

    // List of codes to process
    let code_list: Vec<u32> = vec![510050, 513500, 159659];

    // Process Bars with different strategies
    for &code in &code_list {
        // StrategyA for Bars
        let strategy_a = StrategyType::A(StrategyA);
        let strategy_b = StrategyType::B(StrategyB);
        let mut engine = Engine::<Bar>::new(uri, code, start_date, end_date);
        engine.add_strategy(strategy_a);
        engine.add_strategy(strategy_b);
        engine.run()?;
    }

    // Process Ticks with different strategies
    for &code in &code_list {
        // StrategyA for Ticks
        let strategy_a = StrategyType::A(StrategyA);
        let strategy_b = StrategyType::B(StrategyB);
        let mut engine = Engine::<Tick>::new(uri, code, start_date, end_date);
        engine.add_strategy(strategy_a);
        engine.add_strategy(strategy_b);
        engine.run()?;
    }

    Ok(())
}
````

duckdb backtest engine with multiple `enum`

```rs
use bktrader::datatype::bar::Bar;
use duckdb::{params, Connection};
use std::marker::PhantomData;

// Define the Tick struct
#[derive(Debug)]
struct Tick {
    code: u32,
    open: f64,
    high: f64,
    low: f64,
    close: f64,
}

// Define Strategies
struct StrategyA;
struct StrategyB;

// Trait to handle different data types
trait Strategy<T> {
    fn on_quote(&mut self, quote: &T);
}

// Implement Strategy for StrategyA and StrategyB for Bar
impl Strategy<Bar> for StrategyA {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyA bar: {:?}", quote);
    }
}

impl Strategy<Bar> for StrategyB {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyB bar: {:?}", quote);
    }
}

// Implement Strategy for StrategyA and StrategyB for Tick
impl Strategy<Tick> for StrategyA {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyA tick: {:?}", quote);
    }
}

impl Strategy<Tick> for StrategyB {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyB tick: {:?}", quote);
    }
}

// Trait to map database rows to structs
trait FromRow {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error>
    where
        Self: Sized;
}

// Implement FromRow for Bar
impl FromRow for Bar {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Bar {
            code: row.get(0)?,
            dt: row.get(1)?,
            preclose: row.get(2)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
            netvalue: row.get(7)?,
            volume: row.get(8)?,
            amount: row.get(9)?,
            trades_count: row.get(10)?,
            turnover: row.get(11)?,
        })
    }
}

// Implement FromRow for Tick
impl FromRow for Tick {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Tick {
            code: row.get(0)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
        })
    }
}

enum OnlyBarStrategyType {
    A(StrategyA),
    B(StrategyB),
}

impl Strategy<Bar> for OnlyBarStrategyType {
    fn on_quote(&mut self, quote: &Bar) {
        match self {
            OnlyBarStrategyType::A(s) => s.on_quote(quote),
            OnlyBarStrategyType::B(s) => s.on_quote(quote),
        }
    }
}

enum OnlyTickStrategyType {
    A(StrategyA),
    B(StrategyB),
}

impl Strategy<Tick> for OnlyTickStrategyType {
    fn on_quote(&mut self, quote: &Tick) {
        match self {
            OnlyTickStrategyType::A(s) => s.on_quote(quote),
            OnlyTickStrategyType::B(s) => s.on_quote(quote),
        }
    }
}
enum BothStrategyType {
    A(StrategyA),
    B(StrategyB),
}

impl Strategy<Bar> for BothStrategyType {
    fn on_quote(&mut self, quote: &Bar) {
        match self {
            BothStrategyType::A(s) => s.on_quote(quote),
            BothStrategyType::B(s) => s.on_quote(quote),
        }
    }
}

impl Strategy<Tick> for BothStrategyType {
    fn on_quote(&mut self, quote: &Tick) {
        match self {
            BothStrategyType::A(s) => s.on_quote(quote),
            BothStrategyType::B(s) => s.on_quote(quote),
        }
    }
}

// Generic Engine struct
// PhantomData: To indicate that the Engine struct is generic over T even though it doesn't directly use it.
struct Engine<T, S> {
    uri: String,
    strategies: Vec<S>,
    code: u32,
    start: String,
    end: String,
    _marker: PhantomData<T>,
}

impl<T, S> Engine<T, S>
where
    T: FromRow,
    S: Strategy<T>,
{
    fn new(uri: &str, code: u32, start: &str, end: &str) -> Self {
        Self {
            uri: uri.into(),
            strategies: Vec::with_capacity(8),
            code,
            start: start.into(),
            end: end.into(),
            _marker: PhantomData,
        }
    }

    fn add_strategy(&mut self, stg: S) {
        self.strategies.push(stg);
    }

    fn run(&mut self) -> Result<(), duckdb::Error> {
        let conn = Connection::open(&self.uri)?;
        let query = r#"
            SELECT
                code,
                date_diff('day', DATE '1970-01-01', dt) AS days_since_epoch,
                ROUND(preclose * adjfactor / 1e4, 3) AS adj_preclose,
                ROUND(open * adjfactor / 1e4, 3) AS adj_open,
                ROUND(high * adjfactor / 1e4, 3) AS adj_high,
                ROUND(low * adjfactor / 1e4, 3) AS adj_low,
                ROUND(close * adjfactor / 1e4, 3) AS adj_close,
                ROUND(netvalue * adjfactor / 1e4, 3) AS adj_netvalue,
                volume,
                ROUND(amount * adjfactor / 1e4, 3) AS adj_amount,
                COALESCE(trades_count, 0) AS trades_count,
                turnover
            FROM
                etf
            WHERE
                preclose IS NOT NULL
                AND code = ?
                AND dt BETWEEN CAST(? AS DATE) AND CAST(? AS DATE)
        "#;

        let mut stmt = conn.prepare(&query)?;
        let rows = stmt.query_map(params![self.code, self.start, self.end], |row| T::from_row(row))?;

        for row in rows {
            let data = row?;
            for stg in self.strategies.iter_mut() {
                stg.on_quote(&data);
            }
        }

        Ok(())
    }
}

fn main() -> Result<(), duckdb::Error> {
    // Define the database URI
    let uri = "bar1d.db";
    // Define the date range
    let start_date = "2024-11-20";
    let end_date = "2024-12-31";

    // List of codes to process
    let code_list: Vec<u32> = vec![510050, 513500, 159659];

    // Process Bars with different strategies
    for &code in &code_list {
        let strategy_a = OnlyBarStrategyType::A(StrategyA);
        let strategy_b = OnlyBarStrategyType::B(StrategyB);
        let mut engine = Engine::<Bar, OnlyBarStrategyType>::new(uri, code, start_date, end_date);
        engine.add_strategy(strategy_a);
        engine.add_strategy(strategy_b);
        engine.run()?;
    }

    // Process Ticks with different strategies
    for &code in &code_list {
        let strategy_a = OnlyTickStrategyType::A(StrategyA);
        let strategy_b = OnlyTickStrategyType::B(StrategyB);
        let mut engine = Engine::<Tick, OnlyTickStrategyType>::new(uri, code, start_date, end_date);
        engine.add_strategy(strategy_a);
        engine.add_strategy(strategy_b);
        engine.run()?;
    }

    // Process Ticks with different strategies
    for &code in &code_list {
        let strategy_a = BothStrategyType::A(StrategyA);
        let strategy_b = BothStrategyType::B(StrategyB);
        let mut engine = Engine::<Bar, BothStrategyType>::new(uri, code, start_date, end_date);
        engine.add_strategy(strategy_a);
        engine.add_strategy(strategy_b);
        engine.run()?;
    }

    for &code in &code_list {
        let strategy_a = BothStrategyType::A(StrategyA);
        let strategy_b = BothStrategyType::B(StrategyB);
        let mut engine = Engine::<Tick, BothStrategyType>::new(uri, code, start_date, end_date);
        engine.add_strategy(strategy_a);
        engine.add_strategy(strategy_b);
        engine.run()?;
    }

    Ok(())
}
```

duckdb backtest engine by **trait objects** with **dynamic dispatch**
- Scalability: Easily add new strategies without modifying enums or additional boilerplate code.
- Maintainability: Reduce complexity by leveraging Rust's trait system and dynamic dispatch.
- Flexibility: Allow strategies to implement multiple `Strategy<T>` traits and be reused across different data types.
- Trait objects introduce a slight runtime cost due to dynamic dispatch. However, in most applications, this overhead is negligible compared to the benefits in flexibility and maintainability.
- Type Safety: Rust ensures that only strategies implementing the correct `Strategy<T>`trait can be added to the corresponding `Engine<T>`, maintaining type safety.

```rs
use bktrader::datatype::bar::Bar;
use duckdb::{params, Connection};
use std::marker::PhantomData;

// Define the Tick struct
#[derive(Debug)]
struct Tick {
    code: u32,
    open: f64,
    high: f64,
    low: f64,
    close: f64,
}

// Define Strategies
struct StrategyA;
struct StrategyB;

// Trait to handle different data types
trait Strategy<T> {
    fn on_quote(&mut self, quote: &T);
}

// Implement Strategy for StrategyA and StrategyB for Bar
impl Strategy<Bar> for StrategyA {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyA bar: {:?}", quote);
    }
}

impl Strategy<Bar> for StrategyB {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyB bar: {:?}", quote);
    }
}

// Implement Strategy for StrategyA and StrategyB for Tick
impl Strategy<Tick> for StrategyA {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyA tick: {:?}", quote);
    }
}

impl Strategy<Tick> for StrategyB {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyB tick: {:?}", quote);
    }
}

// Trait to map database rows to structs
trait FromRow {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error>
    where
        Self: Sized;
}

// Implement FromRow for Bar
impl FromRow for Bar {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Bar {
            code: row.get(0)?,
            dt: row.get(1)?,
            preclose: row.get(2)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
            netvalue: row.get(7)?,
            volume: row.get(8)?,
            amount: row.get(9)?,
            trades_count: row.get(10)?,
            turnover: row.get(11)?,
        })
    }
}

// Implement FromRow for Tick
impl FromRow for Tick {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Tick {
            code: row.get(0)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
        })
    }
}

// Generic Engine struct with trait object
// PhantomData: To indicate that the Engine struct is generic over T even though it doesn't directly use it.
struct Engine<T> {
    uri: String,
    strategies: Vec<Box<dyn Strategy<T>>>,
    code: u32,
    start: String,
    end: String,
    _marker: PhantomData<T>,
}

impl<T> Engine<T>
where
    T: FromRow,
{
    fn new(uri: &str, code: u32, start: &str, end: &str) -> Self {
        Self {
            uri: uri.into(),
            strategies: Vec::with_capacity(8),
            code,
            start: start.into(),
            end: end.into(),
            _marker: PhantomData,
        }
    }

    fn add_strategy(&mut self, stg: Box<dyn Strategy<T>>) {
        self.strategies.push(stg);
    }

    fn run(&mut self) -> Result<(), duckdb::Error> {
        let conn = Connection::open(&self.uri)?;
        let query = r#"
            SELECT
                code,
                date_diff('day', DATE '1970-01-01', dt) AS days_since_epoch,
                ROUND(preclose * adjfactor / 1e4, 3) AS adj_preclose,
                ROUND(open * adjfactor / 1e4, 3) AS adj_open,
                ROUND(high * adjfactor / 1e4, 3) AS adj_high,
                ROUND(low * adjfactor / 1e4, 3) AS adj_low,
                ROUND(close * adjfactor / 1e4, 3) AS adj_close,
                ROUND(netvalue * adjfactor / 1e4, 3) AS adj_netvalue,
                volume,
                ROUND(amount * adjfactor / 1e4, 3) AS adj_amount,
                COALESCE(trades_count, 0) AS trades_count,
                turnover
            FROM
                etf
            WHERE
                preclose IS NOT NULL
                AND code = ?
                AND dt BETWEEN CAST(? AS DATE) AND CAST(? AS DATE)
        "#;

        let mut stmt = conn.prepare(&query)?;
        let rows = stmt.query_map(params![self.code, self.start, self.end], |row| T::from_row(row))?;

        for row in rows {
            let data = row?;
            for stg in self.strategies.iter_mut() {
                stg.on_quote(&data);
            }
        }

        Ok(())
    }
}

fn main() -> Result<(), duckdb::Error> {
    // Define the database URI
    let uri = "bar1d.db";
    // Define the date range
    let start_date = "2024-11-20";
    let end_date = "2024-12-31";

    // List of codes to process
    let code_list: Vec<u32> = vec![510050, 513500, 159659];

    // Process Bars with different strategies
    for &code in &code_list {
        let mut engine = Engine::<Bar>::new(uri, code, start_date, end_date);
        engine.add_strategy(Box::new(StrategyA));
        engine.add_strategy(Box::new(StrategyB));
        engine.run()?;
    }

    // Process Ticks with different strategies
    for &code in &code_list {
        let mut engine = Engine::<Tick>::new(uri, code, start_date, end_date);
        engine.add_strategy(Box::new(StrategyA));
        engine.add_strategy(Box::new(StrategyB));
        engine.run()?;
    }

    Ok(())
}
```

duckdb with rayon for strategies & codes

```rs
use bktrader::datatype::bar::Bar;
use duckdb::{params, Connection};
use rayon::prelude::*;
use std::marker::PhantomData;

// Define the Tick struct
#[derive(Debug)]
struct Tick {
    code: u32,
    open: f64,
    high: f64,
    low: f64,
    close: f64,
}

// Define Strategies
struct StrategyA;
struct StrategyB;
// Add more strategies as needed (StrategyC, StrategyD, ..., StrategyZ)

// The strategies are stored as Box<dyn Strategy<T>> and are accessed concurrently.
// To safely send and reference these boxed strategies across threads, they must implement both Send and Sync.
trait Strategy<T>: Send + Sync {
    fn on_quote(&mut self, quote: &T);
}

// Implement Strategy for StrategyA and StrategyB for Bar
impl Strategy<Bar> for StrategyA {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyA bar: {:?}", quote);
    }
}

impl Strategy<Bar> for StrategyB {
    fn on_quote(&mut self, quote: &Bar) {
        println!("StrategyB bar: {:?}", quote);
    }
}

// Implement Strategy for StrategyA and StrategyB for Tick
impl Strategy<Tick> for StrategyA {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyA tick: {:?}", quote);
    }
}

impl Strategy<Tick> for StrategyB {
    fn on_quote(&mut self, quote: &Tick) {
        println!("StrategyB tick: {:?}", quote);
    }
}

// Trait to map database rows to structs
trait FromRow {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error>
    where
        Self: Sized;
}

// Implement FromRow for Bar
impl FromRow for Bar {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Bar {
            code: row.get(0)?,
            dt: row.get(1)?,
            preclose: row.get(2)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
            netvalue: row.get(7)?,
            volume: row.get(8)?,
            amount: row.get(9)?,
            trades_count: row.get(10)?,
            turnover: row.get(11)?,
        })
    }
}

// Implement FromRow for Tick
impl FromRow for Tick {
    fn from_row(row: &duckdb::Row) -> Result<Self, duckdb::Error> {
        Ok(Tick {
            code: row.get(0)?,
            open: row.get(3)?,
            high: row.get(4)?,
            low: row.get(5)?,
            close: row.get(6)?,
        })
    }
}

// Generic Engine struct
// PhantomData: To indicate that the Engine struct is generic over T even though it doesn't directly use it.
struct Engine<T> {
    uri: String,
    strategies: Vec<Box<dyn Strategy<T>>>,
    code: u32,
    start: String,
    end: String,
    _marker: PhantomData<T>,
}

// When using Rayon’s par_iter_mut(), the data (&T) is accessed concurrently across multiple threads.
// To ensure thread safety, T must implement the Sync trait, allowing safe references to T to be shared between threads.
impl<T> Engine<T>
where
    T: FromRow + std::marker::Sync,
{
    fn new(uri: &str, code: u32, start: &str, end: &str) -> Self {
        Self {
            uri: uri.into(),
            strategies: Vec::with_capacity(1000), // Adjust capacity as needed
            code,
            start: start.into(),
            end: end.into(),
            _marker: PhantomData,
        }
    }

    fn add_strategy(&mut self, stg: Box<dyn Strategy<T>>) {
        self.strategies.push(stg);
    }

    fn run(&mut self) -> Result<(), duckdb::Error> {
        let conn = Connection::open(&self.uri)?;
        let query = r#"
            SELECT
                code,
                date_diff('day', DATE '1970-01-01', dt) AS days_since_epoch,
                ROUND(preclose * adjfactor / 1e4, 3) AS adj_preclose,
                ROUND(open * adjfactor / 1e4, 3) AS adj_open,
                ROUND(high * adjfactor / 1e4, 3) AS adj_high,
                ROUND(low * adjfactor / 1e4, 3) AS adj_low,
                ROUND(close * adjfactor / 1e4, 3) AS adj_close,
                ROUND(netvalue * adjfactor / 1e4, 3) AS adj_netvalue,
                volume,
                ROUND(amount * adjfactor / 1e4, 3) AS adj_amount,
                COALESCE(trades_count, 0) AS trades_count,
                turnover
            FROM
                etf
            WHERE
                preclose IS NOT NULL
                AND code = ?
                AND dt BETWEEN CAST(? AS DATE) AND CAST(? AS DATE)
        "#;

        let mut stmt = conn.prepare(&query)?;
        let rows = stmt.query_map(params![self.code, self.start, self.end], |row| T::from_row(row))?;

        for row in rows {
            let data = row?;
            // Parallel execution of strategies using Rayon
            self.strategies.par_iter_mut().for_each(|stg| stg.on_quote(&data));
        }

        Ok(())
    }
}

fn main() -> Result<(), duckdb::Error> {
    // Define the database URI
    let uri = "bar1d.db";
    // Define the date range
    let start = "2024-11-20";
    let end = "2024-12-31";

    // List of codes to process
    let code_list: Vec<u32> = vec![510050, 513500, 159659];

    code_list.par_iter().for_each(|&code| {
        let mut engine = Engine::<Bar>::new(uri, code, start, end);
        engine.add_strategy(Box::new(StrategyA));
        engine.add_strategy(Box::new(StrategyB));
        if let Err(e) = engine.run() {
            eprintln!("Error processing Bar code {}: {:?}", code, e);
        }
    });

    Ok(())
}
```

To further optimize your system by parallelizing the processing of **5000 codes** in addition to the existing parallelization of **1000 strategies**, you can leverage Rayon’s powerful parallel iterators.

However, it's essential to approach this carefully to ensure efficient resource utilization and avoid potential performance bottlenecks due to **nested parallelism** (parallelizing both codes and strategies simultaneously).

Below, I’ll guide you through the necessary steps to parallelize both levels effectively:

1. **Understand the Current Parallelization:**
   - **Strategies Parallelization:** You’re already using Rayon to execute 1000 strategies in parallel for each code.
   - **Codes Parallelization:** Now, you want to process 5000 codes concurrently, each running its own set of strategies.

2. **Challenges with Nested Parallelism:**
   - **Resource Contention:** Parallelizing both codes and strategies can lead to excessive thread creation, causing CPU and memory contention.
   - **Performance Overhead:** Nested parallelism might introduce overheads that negate the benefits of parallel execution.

3. **Solution: Single-Level Parallelism with Task Batching**
   
   Instead of parallelizing both codes and strategies simultaneously, you can **parallelize the outer loop (codes)** and keep the **inner loop (strategies)** sequential or vice versa. This approach prevents excessive thread creation and manages resources efficiently.

   Given that strategies are already parallelized, and considering the high number of codes (5000), it's more manageable to parallelize the **codes** and execute strategies **sequentially** within each code's processing. Alternatively, you can manage parallelism depth by adjusting Rayon’s thread pool.

   For simplicity and efficiency, I'll demonstrate **parallelizing the codes** while keeping the **strategies parallelized** within each code. This approach leverages Rayon’s work-stealing scheduler to manage the workload effectively.
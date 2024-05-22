# SQLite

- [SQLite](#sqlite)
  - [write to sqlite by a list of dicts](#write-to-sqlite-by-a-list-of-dicts)

## write to sqlite by a list of dicts

```py
def process_data(dicts: list[dict]):
    conn_ = sqlite3.connect("test.db")
    conn_.execute(
        """
        CREATE TABLE IF NOT EXISTS bar1m (
            code TEXT, 
            date INTEGER, 
            time INTEGER, 
            open INTEGER, 
            high INTEGER, 
            low INTEGER, 
            close INTEGER, 
            num_trades INTEGER, 
            volume INTEGER, 
            amount INTEGER
        );
        """
    )
    # insert data
    conn_.executemany("INSERT INTO bar1m VALUES(:code, :date, :time, :open, :high, :low, :close, :num_trades, :volume, :amount);", dicts)
    conn_.commit()
```
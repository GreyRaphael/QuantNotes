# polars

- [polars](#polars)
  - [deltalake](#deltalake)
  - [tips](#tips)
  - [group dynamic](#group-dynamic)

## deltalake

> In fact, deltalake is composed of many *.parquet files with unique name

`pip install deltalake`

[write_delta_tutorial](https://blog.devgenius.io/harnessing-the-power-of-polars-and-delta-lake-for-data-processing-2d285ccfbef7)

```py
import polars as pl
import deltalake

opt=deltalake.WriterProperties(compression='zstd', compression_level=22)

df.write_delta('compare/delta_zstd', delta_write_options={
        "engine": "rust",
        "writer_properties": opt,
    })
```

## tips

for stock kl1m, tick, order/trade:
- `pl.write_ipc(filname, compression='zstd')` is quick and smaller than 
- `pl.write_parquet(filename, compression='zstd', compression_level=22)`

## group dynamic

```py
import polars as pl

df = pl.DataFrame(
    {
        "code": [
            "000001",
            "000001",
            "000001",
            "000001",
            "000001",
            "000001",
            "000002",
            "000002",
            "000002",
            "000002",
            "000002",
            "000002",
        ],
        "time": [
            "2021-01-01 09:00:00",
            "2021-01-01 09:01:00",
            "2021-01-01 09:02:00",
            "2021-01-01 09:03:00",
            "2021-01-01 09:04:00",
            "2021-01-02 09:05:00",
            "2021-01-01 09:00:00",
            "2021-01-01 09:01:00",
            "2021-01-01 09:02:00",
            "2021-01-01 09:03:00",
            "2021-01-01 09:04:00",
            "2021-01-01 09:05:00",
        ],
        "price": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12],
        "volume": [101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112],
    }
).with_columns(pl.col("time").str.to_datetime("%Y-%m-%d %H:%M:%S"))

# simple solution
df.group_by_dynamic('time', every='3m', group_by=['code', pl.col('time').dt.date().alias('date')]).agg(pl.col('volume', 'price').last())

# complicated solution
df.with_columns(pl.col("time").dt.date().alias("date")).group_by(by=['code', 'date']).map_groups(lambda x: x.sort('time').group_by_dynamic('time', every='3m').agg(pl.col('volume', 'price', 'code').last()))
```
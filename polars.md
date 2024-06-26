# polars

- [polars](#polars)
  - [deltalake](#deltalake)
  - [tips](#tips)
  - [temporal](#temporal)
    - [`dt.round`](#dtround)
    - [`dt.truncate`](#dttruncate)
    - [group dynamic](#group-dynamic)
    - [`join_asof`](#join_asof)
  - [ipc](#ipc)

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

## temporal

### `dt.round`

```py
import polars as pl
import pyarrow.compute as pc

df = pl.DataFrame(
    {
        "dt": [
            "2024-05-31 09:30:02",
            "2024-05-31 09:30:05",
            "2024-05-31 09:30:08",
            "2024-05-31 09:30:11",
            "2024-05-31 09:30:14",
        ]
    }
).with_columns(pl.col("dt").str.to_datetime())

df.select(pl.col('dt').dt.round(every='3s').dt.offset_by('-3s'))
# 2024-05-31 09:30:00
# 2024-05-31 09:30:03
# 2024-05-31 09:30:06
# 2024-05-31 09:30:09
# 2024-05-31 09:30:12

# by pyarrow roumd_temporal
dt_arr = df.to_arrow()["dt"]
pc.round_temporal(dt_arr, multiple=3, unit='second')
# 2024-05-31 09:30:03
# 2024-05-31 09:30:06
# 2024-05-31 09:30:09
# 2024-05-31 09:30:12
# 2024-05-31 09:30:15
```

### `dt.truncate`

```py
import polars as pl

df = pl.DataFrame(
    {
        "dt": [
            "2024-01-02 09:30:41",
            "2024-01-02 09:30:43",
            "2024-01-02 09:30:47",
            "2024-01-02 09:30:50",
            "2024-01-02 09:30:52",
            "2024-01-02 10:54:56",
            "2024-01-02 10:55:00",
            "2024-01-02 10:55:02",
            "2024-01-02 10:55:05",
            "2024-01-02 10:55:11",
        ]
    }
).with_columns(pl.col("dt").str.to_datetime())

df.with_columns(pl.col("dt").dt.round(every="3s").alias("dt_round"))
# 2024-01-02 09:30:41	2024-01-02 09:30:42
# 2024-01-02 09:30:43	2024-01-02 09:30:42
# 2024-01-02 09:30:47	2024-01-02 09:30:48
# 2024-01-02 09:30:50	2024-01-02 09:30:51
# 2024-01-02 09:30:52	2024-01-02 09:30:51
# 2024-01-02 10:54:56	2024-01-02 10:54:57
# 2024-01-02 10:55:00	2024-01-02 10:55:00
# 2024-01-02 10:55:02	2024-01-02 10:55:03
# 2024-01-02 10:55:05	2024-01-02 10:55:06
# 2024-01-02 10:55:11	2024-01-02 10:55:12

df.with_columns(pl.col("dt").dt.truncate(every="3s").alias("dt_round"))
# 2024-01-02 09:30:41	2024-01-02 09:30:39
# 2024-01-02 09:30:43	2024-01-02 09:30:42
# 2024-01-02 09:30:47	2024-01-02 09:30:45
# 2024-01-02 09:30:50	2024-01-02 09:30:48
# 2024-01-02 09:30:52	2024-01-02 09:30:51
# 2024-01-02 10:54:56	2024-01-02 10:54:54
# 2024-01-02 10:55:00	2024-01-02 10:55:00
# 2024-01-02 10:55:02	2024-01-02 10:55:00
# 2024-01-02 10:55:05	2024-01-02 10:55:03
# 2024-01-02 10:55:11	2024-01-02 10:55:09
```

### group dynamic

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

### `join_asof`

> left-join except that we match on nearest key rather than equal keys.

```py
import polars as pl
import datetime as dt

df1 = pl.DataFrame(
    {
        "dt": [dt.datetime(2024, 1, 1, 8, 1), dt.datetime(2024, 1, 1, 8, 3), dt.datetime(2024, 1, 1, 8, 6), dt.datetime(2024, 1, 1, 8, 7)],
        "val": [1, 2, 3, 4],
    }
).sort("dt")
# 2024-01-01 08:01:00	1
# 2024-01-01 08:03:00	2
# 2024-01-01 08:06:00	3
# 2024-01-01 08:07:00	4

df2 = pl.DataFrame(
    {
        "dt": [dt.datetime(2024, 1, 1, 8, 0), dt.datetime(2024, 1, 1, 8, 3), dt.datetime(2024, 1, 1, 8, 6), dt.datetime(2024, 1, 1, 8, 9)],
    }
).sort("dt")
# 2024-01-01 08:00:00
# 2024-01-01 08:03:00
# 2024-01-01 08:06:00
# 2024-01-01 08:09:00

df2.join_asof(df1, on="dt", tolerance="3m", strategy="nearest")
# 2024-01-01 08:00:00	1
# 2024-01-01 08:03:00	2
# 2024-01-01 08:06:00	3
# 2024-01-01 08:09:00	4
```

## ipc

[polars.DataFrame.write_ipc](https://docs.pola.rs/api/python/stable/reference/api/polars.DataFrame.write_ipc.html) not suport `compression_level`

```py
import polars as pl

df = pl.DataFrame(
    {
        "foo": [1, 2, 3, 4, 5],
        "bar": [6, 7, 8, 9, 10],
        "ham": ["a", "b", "c", "d", "e"],
    }
)
df.write_ipc("test.arrow", compression="zstd")
```

convert to `pyarrow.Table` and write file using [pyarrow.ipc.RecordBatchFileWriter](https://arrow.apache.org/docs/python/generated/pyarrow.ipc.RecordBatchFileWriter.html)

```py
import polars as pl
import pyarrow as pa

df = pl.DataFrame(
    {
        "foo": [1, 2, 3, 4, 5],
        "bar": [6, 7, 8, 9, 10],
        "ham": ["a", "b", "c", "d", "e"],
    }
)

tb = df.to_arrow()

opt = pa.ipc.IpcWriteOptions(compression=pa.Codec(compression="zstd", compression_level=22))

with pa.ipc.new_file("test2.arrow", schema=tb.schema, options=opt) as writer:
    writer.write_table(tb)
    # writer.write(tb) # alternative
```

convert to `pyarrow.RecordBatch` and write to ipc file

```py
import polars as pl
import pyarrow as pa

df = pl.DataFrame(
    {
        "foo": [1, 2, 3, 4, 5],
        "bar": [6, 7, 8, 9, 10],
        "ham": ["a", "b", "c", "d", "e"],
    }
)

batches=df.to_arrow().to_batches()

opt = pa.ipc.IpcWriteOptions(compression=pa.Codec(compression="zstd", compression_level=22))

with pa.ipc.new_file("test2.arrow", schema=batches[0].schema, options=opt) as writer:
    for rb in batches:
        writer.write_batch(rb)
        # writer.write(rb) # alternative
```
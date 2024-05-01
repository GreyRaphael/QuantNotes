# polars

- [polars](#polars)
  - [deltalake](#deltalake)
  - [tips](#tips)

## deltalake

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


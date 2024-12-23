# Introduction

- [Introduction](#introduction)
  - [kline](#kline)

## kline

from bar1m to bar5m

```py
import polars as pl

df = pl.read_ipc("etf-kl1m/2024/*.ipc")
df30min=df.group_by_dynamic("dt", every="30m", period="30m", closed="left", label="right", group_by="code").agg(
    [
        pl.first("open"),
        pl.max("high"),
        pl.min("low"),
        pl.last("last"),
        pl.sum("num_trades"),
        pl.sum("volume"),
        pl.sum("amount"),
    ]
).sort(by=['code','dt'])
```
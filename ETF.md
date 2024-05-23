# ETF

- [ETF](#etf)
  - [fields](#fields)


## fields

discount_rate= 100 * ( close - netvalue ) / netvalue


turnover = volume / total_share

```py
def merge_etf_bar1d(year: int):
    """join etf bar1d between aiquant & wind"""
    df_wind = pl.read_ipc(f"wind_bar1d_etf/{year}.ipc", memory_map=False)
    df_ai = pl.read_ipc(f"aiquant-ETF-bar1d/{year}.ipc", memory_map=False)
    df_final = df_ai.join(df_wind.select("code", "dt", "turnover", (pl.col("netvalue") * 1e4).round(0).cast(pl.UInt32)), on=["code", "dt"], how="inner")
    df_final.sort(["code", "dt"]).write_ipc(f"wind_aiquant_etf_bar1d/{year}.ipc", compression="zstd")
    print("finish", year)
```
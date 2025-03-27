# Introduction

- [Introduction](#introduction)
  - [stock randomness](#stock-randomness)
    - [some kinds of random walk](#some-kinds-of-random-walk)
    - [how to determine the white noise](#how-to-determine-the-white-noise)
    - [random walk strategy](#random-walk-strategy)
  - [kline](#kline)
  - [stock indicators](#stock-indicators)

## stock randomness

- [股价是随机游走的吗](https://zhuanlan.zhihu.com/p/115342322)
- [A股股票序列是随机游走吗](https://zhuanlan.zhihu.com/p/146905803)

### some kinds of random walk

$\epsilon_t$ is white noise, it meets the following conditions:
1. $E(\epsilon_t)=0$, average is 0
2. $D(\epsilon_t)=\sigma^2<\infty$, variance is finite fixed value
3. $Cov(\epsilon_t, \epsilon_s)=0$, autocorrelation is 0

some kinds of random process:
- diff following normal distribution:

$$X_t = X_{t-1} + \epsilon_t$$

$$(X_t - X_{t-1}) \sim N(0, \sigma^2)$$

- divede following normal distribution:

$$X_t = X_{t-1} \times \epsilon_t$$

$$\frac{X_t}{X_{t-1}} \sim N(0, \sigma^2)$$

- binomial distribution:

$$X_t = X_{t-1} \times \epsilon_t, \epsilon_t \sim Bin(n, 0.5)$$

$$\frac{X_t}{X_{t-1}} \sim Bin(n, 0.5)$$

### how to determine the white noise

How to determine the randomness is white noise? **Ljung-Box test**
> `pip install statsmodels`

```py
import numpy as np
import statsmodels.api as sm
from statsmodels.stats.diagnostic import acorr_ljungbox

# Generate white noise data
np.random.seed(0)
data = np.random.normal(loc=0, scale=1, size=1000)

# Perform Ljung-Box test, choose three lags: 5, 10, 20
test_results = acorr_ljungbox(data, lags=[5,10,20], return_df=True)
# This will output a DataFrame with the test statistics and corresponding p-values for each lag.
print(test_results)

#     lb_stat  lb_pvalue
# 5   7.8952   0.3422
# 10  8.8952   0.5422
# 20  9.8952   0.7422
# lb_pvalue > 0.05, so it is white noise
```

- **Null Hypothesis (H₀)**: The data are independently distributed (i.e., no autocorrelation).
- **Alternative Hypothesis (H₁)**: The data are not independently distributed (i.e., exhibit autocorrelation).

Interpretation: 
- If the p-value is **greater than** your chosen significance level (commonly **0.05**), you fail to reject the null hypothesis, suggesting that the time series may be white noise. 
- Conversely, a p-value **less than 0.05** indicates significant autocorrelation, implying the series is not white noise.

### random walk strategy

- A股股票的价格$Price$不符合random walk,
- $Price_t-Price_{t-1}$是可能符合random walk
- $Price_t/Price_{t-1}$是可能符合random walk

If stock returns follow a random walk, meaning their future movements are unpredictable and independent of past behavior, traditional trading strategies that rely on forecasting price directions become ineffective. In such scenarios, the following approaches are often recommended:

1. **Passive Investment Strategy**:
   - **Diversified Index Funds**: Investing in broad market index funds or exchange-traded funds (ETFs) allows investors to capture the overall market performance without attempting to predict individual stock movements. This approach aligns with the random walk theory, which suggests that stock prices move unpredictably, making it challenging to outperform the market through active trading. 

2. **Buy-and-Hold Strategy**:
   - **Long-Term Holding**: By purchasing a diversified portfolio of assets and holding them over an extended period, investors can benefit from the general upward trend of the market, despite short-term volatility. This strategy reduces transaction costs and minimizes the risks associated with frequent trading. 

3. **Grid Trading Strategy**:
   - **Mechanics**: Grid trading involves placing buy and sell orders at predetermined intervals above and below a set price, creating a "grid" of orders. This strategy doesn't rely on predicting market direction but instead capitalizes on market volatility by executing trades as prices fluctuate within the grid.
   - **Considerations**: While grid trading can be profitable in ranging markets, it carries risks, especially in trending markets where prices move consistently in one direction. It's essential to implement risk management measures, such as stop-loss orders, to mitigate potential losses.

**Key Considerations**:

- **Market Efficiency**: The random walk theory is closely related to the Efficient Market Hypothesis (EMH), which posits that asset prices fully reflect all available information. Under EMH, consistently outperforming the market through active trading is challenging, reinforcing the appeal of passive investment strategies. 

- **Risk Management**: Regardless of the chosen strategy, implementing robust risk management practices is crucial. This includes setting appropriate position sizes, using stop-loss orders, and regularly reviewing and adjusting the investment portfolio to align with financial goals and risk tolerance.

In summary, if stock returns adhere to a random walk, passive investment strategies like investing in diversified index funds or employing a buy-and-hold approach are generally considered effective. While grid trading can offer opportunities in certain market conditions, it requires careful implementation and risk management to be successful. 


## kline

from tick to bar1m, then from bar1m to bar5m, bar10m, bar30m, bar1h bar2h

```py
import polars as pl
import datetime as dt


def prepare_bar1m(target_dt: dt.date) -> pl.DataFrame:
    target_filename = target_dt.strftime("%Y%m%d.ipc")
    df = pl.read_ipc(target_filename)  # read tick, df is sorted by [code, dt]

    # duplicate the first row of each code to 8:00
    df_first = (
        df.group_by("code")
        .first()
        .with_columns(
            pl.col("dt").dt.replace(hour=8),
            volume=pl.lit(0, pl.UInt64),
            amount=pl.lit(0, pl.UInt64),
            num_trades=pl.lit(0, pl.UInt32),
        )
    )

    # change >=11:30:00 to 11:29:59.999
    # change >=15:00:00 to 14:59:59.999
    df_body = df.with_columns(
        pl.when(
            ((pl.col("dt").dt.hour() == 11) & (pl.col("dt").dt.minute() == 30)) | ((pl.col("dt").dt.hour() == 15) & (pl.col("dt").dt.minute() == 0)),
        )
        .then(
            pl.col("dt").dt.replace(second=0, microsecond=0) - dt.timedelta(milliseconds=1),
        )
        .otherwise("dt")
        .alias("dt")
    )

    # concat the two dataframes
    dff = pl.concat([df_first, df_body]).sort(by=["code", "dt"])
    # group by 1 minute

    df_bar1m = (
        dff.group_by_dynamic("dt", every="1m", period="1m", closed="left", label="right", group_by="code")
        .agg(
            [
                pl.first("preclose").alias("preclose"),
                pl.first("last").alias("open"),
                pl.max("last").alias("high"),
                pl.min("last").alias("low"),
                pl.last("last").alias("close"),
                pl.last("volume"),
                pl.last("amount"),
                pl.last("num_trades"),
            ]
        )
        .sort(by=["code", "dt"])
        .with_columns(
            pl.col("volume").diff().over("code"),
            pl.col("amount").diff().over("code"),
            pl.col("num_trades").diff().over("code"),
        )
        .filter(pl.col("volume").is_not_null())
    )

    # time base
    # [09:31, ...11:30]
    # [13:01, ...15:00]
    dft = pl.concat(
        [
            pl.Series([dt.datetime.combine(target_dt, dt.time(9, 26))], dtype=pl.Datetime(time_unit="ms")),
            pl.datetime_range(
                start=dt.datetime.combine(target_dt, dt.time(9, 30)),
                end=dt.datetime.combine(target_dt, dt.time(11, 30)),
                closed="right",
                interval="1m",
                time_unit="ms",
                eager=True,
            ),
            pl.datetime_range(
                start=dt.datetime.combine(target_dt, dt.time(13, 0)),
                end=dt.datetime.combine(target_dt, dt.time(15, 0)),
                closed="right",
                interval="1m",
                time_unit="ms",
                eager=True,
            ),
        ]
    ).to_frame(name="dt")
    # Cartesian product of uniqute code x datetime
    dft_base = df.select(pl.col("code").unique()).join(dft, how="cross")

    # df_bar1m aligned
    df_aligned_bar1m = (
        dft_base.join(df_bar1m, on=["code", "dt"], how="left")
        .with_columns(
            pl.col("code").fill_null(strategy="forward").fill_null(strategy="backward").over("code"),
            pl.col("close").fill_null(strategy="forward").fill_null(strategy="backward").over("code"),
            pl.col("volume").fill_null(0).over("code"),
            pl.col("amount").fill_null(0).over("code"),
            pl.col("num_trades").fill_null(0).over("code"),
        )
        .with_columns(
            pl.col("open").fill_null(pl.col("close")).over("code"),
            pl.col("high").fill_null(pl.col("close")).over("code"),
            pl.col("low").fill_null(pl.col("close")).over("code"),
        )
    )
    return df_aligned_bar1m

# bar1m to bar5m, bar10m, bar30m, bar1h, bar2h
def generate_bar(df_aligned_bar1m: pl.DataFrame, minute_interval: int = 10) -> pl.DataFrame:
    df_bar = (
        df_aligned_bar1m.filter(pl.col("dt").dt.time() > dt.time(9, 30))
        .with_columns(
            ((pl.cum_count("dt") - 1).over("code") // minute_interval).alias("count"),
        )
        .group_by(
            ["code", "count"],
            maintain_order=True,
        )
        .agg(
            [
                pl.last("dt"),
                pl.first("preclose"),
                pl.first("open"),
                pl.max("high"),
                pl.min("low"),
                pl.last("close"),
                pl.sum("volume"),
                pl.sum("amount"),
                pl.sum("num_trades"),
            ]
        )
        .select(pl.exclude("count"))
    )
    return df_bar
```

## stock indicators

[Spearman's rank correlation of technical indicators](https://grzegorz.link/indicators)

[谈谈技术分析的本质](https://mp.weixin.qq.com/s/OUA2PfwpCWJCu2CGhTDmvw)
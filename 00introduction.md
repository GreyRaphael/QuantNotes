# Introduction

- [Introduction](#introduction)
  - [stock randomness](#stock-randomness)
    - [some kinds of random walk](#some-kinds-of-random-walk)
    - [how to determine the white noise](#how-to-determine-the-white-noise)
    - [random walk strategy](#random-walk-strategy)
  - [kline](#kline)

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

from tick to bar1m

```py
import polars as pl

df = pl.read_ipc("etf-kl1m/2024/*.ipc")
df30min = (
    df.group_by_dynamic("dt", every="1m", period="1m", closed="left", label="right", group_by="code")
    .agg(
        [
            pl.first("last").alias("open"),
            pl.max("last").alias("high"),
            pl.min("last").alias("low"),
            pl.last("last").alias("close"),
            pl.last("volume"),
            pl.last("amount"),
        ]
    )
    .sort(by=["code", "dt"])
    .with_columns(pl.col("volume").diff().over("code"), pl.col("amount").diff().over("code"))
)
```
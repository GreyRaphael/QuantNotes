# Introduction

- [Introduction](#introduction)
  - [stock randomness](#stock-randomness)
    - [some kinds of random walk](#some-kinds-of-random-walk)
    - [how to determine the white noise](#how-to-determine-the-white-noise)

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
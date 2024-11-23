# Introduction

- [Introduction](#introduction)
  - [stock randomness](#stock-randomness)
    - [some kinds of random walk](#some-kinds-of-random-walk)

## stock randomness

- [股价是随机游走的吗](https://zhuanlan.zhihu.com/p/115342322)
- [A股股票序列是随机游走吗](https://zhuanlan.zhihu.com/p/146905803)

How to determine the randomness of a stock price?
> `pip install statsmodels`

```py
import numpy as np
import statsmodels.api as sm
from statsmodels.stats.diagnostic import acorr_ljungbox

# Generate white noise data
np.random.seed(0)
data = np.random.normal(loc=0, scale=1, size=1000)

# Perform Ljung-Box test
test_results = acorr_ljungbox(data, lags=[10], return_df=True)
print(test_results)

#     lb_stat  lb_pvalue
# 10  8.8952   0.5422
# lb_pvalue > 0.05, so it is white noise
```

### some kinds of random walk

$\epsilon_t$ is white noise

$$\epsilon_t, E(\epsilon_t)=0, D(\epsilon_t)=\sigma^2<\infty$$

- diff following normal distribution:

$$X_t = X_{t-1} + \epsilon_t$$

$$(X_t - X_{t-1}) \sim N(0, \sigma^2)$$

- divede following normal distribution:

$$X_t = X_{t-1} \times \epsilon_t$$

$$\frac{X_t}{X_{t-1}} \sim N(0, \sigma^2)$$

- binomial distribution:

$$X_t = X_{t-1} \times \epsilon_t, \epsilon_t \sim Bin(n, 0.5)$$

$$\frac{X_t}{X_{t-1}} \sim Bin(n, 0.5)$$
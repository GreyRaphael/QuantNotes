# Introduction

- [Introduction](#introduction)
  - [stock randomness](#stock-randomness)

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
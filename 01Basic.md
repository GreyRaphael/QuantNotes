# Quantitative Trading Basic

均线:
- Granville
- SMA: 简单移动平均，所有数据点相同权重，计算周期固定，新数据点加入，最旧数据点删除
- WMA: 加权移动平均，新数据点加入，最旧数据点删除，所有权重总和为1，最近的数据权重最大
- EMA: 指数移动平均, 旧数据点不删除，仅仅减少其权重

基金分析：收盘价

股票分析：收盘价，最高价，最低价

Example: basic

```python
from collections import namedtuple, OrderedDict
from collections.abc import Iterable

class Stock(object):
    def __init__(self, price_array, start_date, date_array=None):
        self.__price_array=price_array
        self.__date_array=self._init_days(start_date, date_array)
        self.__change_array=self.__init_change()
        self.stock_dict=self._init_stock_dict()
    
    def _init_days(self, start_date, date_array):
        if date_array is None:
            date_array=[f"{start_date+i}" for i, _ in enumerate(self.__price_array)]
        else:
            date_array=[f"{date}" for date in date_array]
        return date_array
    
    def __init_change(self):
        float_price_array=[float(p) for p in self.__price_array]
        price_tuples=[(price1, price2) for price1, price2 in zip(float_price_array[:-1], float_price_array[1:])]
        change_array=list(map(lambda t: round((t[1]-t[0])/t[0], 3), price_tuples))
        change_array.insert(0,0)
        return change_array
    
    def _init_stock_dict(self):
        stock_namedtuple=namedtuple('stock', ('date', 'price', 'change'))
        stock_dict=OrderedDict((date, stock_namedtuple(date, price, change)) for date, price, change in zip(self.__date_array, self.__price_array, self.__change_array))
        return stock_dict
    
    def filter_stock(self, up=True, sum=False):
        filter_func=(lambda day: day.change>0) if up else (lambda day: day.change<0)
        result=filter(filter_func, self.stock_dict.values())

        if not sum:
            return result
        else:
            sum=0.0
            for day in result:
                sum+=day.change
            return sum
        
    def __str__(self):
        return str(self.stock_dict)
    
    def __iter__(self):
        for k in self.stock_dict:
            yield self.stock_dict[k]
    
    def __len__(self):
        return len(self.stock_dict)
    
    def __getitem__(self, i):
        date_key=self.__date_array[i]
        return self.stock_dict[date_key]

if __name__ == "__main__":
    price_array=['30.14', '29.58', '26.36', '32.56', '32.82']
    start_date=20191025
    s1=Stock(price_array, start_date)
    # __str__
    print(s1)
    # __len__
    print(len(s1))
    # __getitem__
    print(s1[0])
    # __iter__
    if isinstance(s1, Iterable):
        for day in s1:
            print(day)
    
    # test api
    print(list(s1.filter_stock()))
    print(list(s1.filter_stock(up=False)))
    print(s1.filter_stock(up=False, sum=True))
```

Example: simple
> `pip install abupy`

```python
from collections import namedtuple, OrderedDict
from collections.abc import Iterable
from abupy import ABuSymbolPd

class Stock(object):
    pass

if __name__ == "__main__":
    # Tesla 1 year data
    price_array=ABuSymbolPd.make_kl_df('TSLA', n_folds=2).close.tolist()
    date_array=ABuSymbolPd.make_kl_df('TSLA', n_folds=2).date.tolist()
    s1=Stock(price_array, None, date_array)
    # __len__
    print(len(s1))
    # __getitem__
    print(s1[-1])
    print(s1.filter_stock(up=False, sum=True))
```

example: python3 abstact class

```py
from abc import ABC, abstractclassmethod

class TradeStrategyBase(ABC):
    @abstractclassmethod
    def func(self):
        raise NotImplementedError    

class TradeStrategy1(TradeStrategyBase):
    # 必须实现func，否则报错
    def func(self):
        print("world")    

if __name__ == "__main__":
    t=TradeStrategy1()
    t.func()
```

example: simple example with loopback

```py
from collections import namedtuple, OrderedDict
from collections.abc import Iterable
from abc import ABC, abstractclassmethod
import numpy as np
import matplotlib.pyplot as plt


class Stock(object):
    pass


class TradeStrategyBase(ABC):
    @abstractclassmethod
    def buy_strategy(self, *args, **kwargs):
        raise NotImplementedError

    @abstractclassmethod
    def sell_strategy(self, *args, **kwargs):
        raise NotImplementedError


class TradeStrategy1(TradeStrategyBase):
    # 追涨策略，股价上涨超过1%时，买入股票持有10天
    s_keep_stock_threshold = 10

    def __init__(self):
        self.keep_stock_day = 0
        self.__buy_change_threshold = 0.01

    def buy_strategy(self, trade_ind, trade_day, trade_days):
        if self.keep_stock_day == 0 and trade_day.change > self.__buy_change_threshold:
            self.keep_stock_day += 1
        elif self.keep_stock_day > 0:
            self.keep_stock_day += 1

    def sell_strategy(self, trade_ind, trade_day, trade_days):
        if self.keep_stock_day >= TradeStrategy1.s_keep_stock_threshold:
            self.keep_stock_day = 0

    @property
    def buy_change_threshold(self):
        return self.__buy_change_threshold

    @buy_change_threshold.setter
    def buy_change_threshold(self, v):
        if not isinstance(v, float):
            raise TypeError('v must be float')
        self.__buy_change_threshold = round(v, 2)


class TradeLoopBack(object):
    # 回测系统
    def __init__(self, trade_days, trade_strategy):
        self.trade_days = trade_days
        self.trade_strategy = trade_strategy
        self.profit_array = []

    def execute_trade(self):
        for i, d in enumerate(self.trade_days):
            if self.trade_strategy.keep_stock_day > 0:
                self.profit_array.append(d.change)

            if hasattr(self.trade_strategy, 'buy_strategy'):
                self.trade_strategy.buy_strategy(i, d, self.trade_days)

            if hasattr(self.trade_strategy, 'sell_strategy'):
                self.trade_strategy.sell_strategy(i, d, self.trade_days)


if __name__ == "__main__":
    price_array = ['1.6638', '1.679', '1.7159', '1.7245', '1.7639', '1.7356', '1.7313', '1.734', '1.7112', '1.6974', '1.73', '1.7294', '1.7212', '1.7434', '1.7714', '1.7427', '1.7548', '1.7375', '1.7535', '1.7655', '1.8027', '1.7912', '1.7926']
    s1 = Stock(price_array, start_date=20191001)
    # print(s1)
    ts1 = TradeStrategy1()

    trade_loop_back = TradeLoopBack(s1, ts1)
    trade_loop_back.execute_trade()
    print(trade_loop_back.profit_array)
    # 下面没有按照复利计算
    plt.plot(np.array(trade_loop_back.profit_array).cumsum())
    plt.show()
```

example: find good parameter

```py
from collections import namedtuple, OrderedDict
from collections.abc import Iterable
from abc import ABC, abstractclassmethod
import numpy as np
import matplotlib.pyplot as plt
from functools import reduce
from itertools import product

class Stock(object):
    pass


class TradeStrategyBase(ABC):
    pass


class TradeStrategy2(TradeStrategyBase):
    # 追跌策略，连续下跌两天，下跌率之和<-0.01，买入持有10天
    s_keep_stock_threshold = 10
    s_buy_change_threshold = -0.01

    def __init__(self):
        self.keep_stock_day = 0

    def buy_strategy(self, trade_ind, trade_day, trade_days):
        if self.keep_stock_day == 0 and trade_ind >= 1:
            today_down = trade_day.change
            yesterday_down = trade_days[trade_ind-1].change
            if today_down < 0 and yesterday_down < 0 and today_down+yesterday_down < TradeStrategy2.s_buy_change_threshold:
                self.keep_stock_day += 1
        elif self.keep_stock_day > 0:
            self.keep_stock_day += 1

    def sell_strategy(self, trade_ind, trade_day, trade_days):
        if self.keep_stock_day >= TradeStrategy2.s_keep_stock_threshold:
            self.keep_stock_day = 0

    @classmethod
    def set_keep_stock_threshold(cls, v):
        cls.s_keep_stock_threshold = v

    @staticmethod
    def set_buy_change_threshold(v):
        TradeStrategy2.s_buy_change_threshold = v


class TradeLoopBack(object):
    pass


def calc(trade_days, keep_stock_threshold, buy_change_threshold):
    ts = TradeStrategy2()
    TradeStrategy2.set_keep_stock_threshold(keep_stock_threshold)
    TradeStrategy2.set_buy_change_threshold(buy_change_threshold)

    trade_loopback = TradeLoopBack(trade_days, ts)
    trade_loopback.execute_trade()
    
    profit = 0 if len(trade_loopback.profit_array) == 0 else reduce(
        lambda a, b: a+b, trade_loopback.profit_array)
    return profit, keep_stock_threshold, buy_change_threshold


if __name__ == "__main__":
    price_array = ['1.6638', '1.679', '1.7159', '1.7245', '1.7639', '1.7356', '1.7313', '1.734', '1.7112', '1.6974', '1.73',
                  '1.7294', '1.7212', '1.7434', '1.7714', '1.7427', '1.7548', '1.7375', '1.7535', '1.7655', '1.8027', '1.7912', '1.7926']
    s1 = Stock(price_array, start_date=20191001)
    # print(s1)
    # optimize parameter: keep_stock_threshold, buy_change_threshold
    keep_stock_list=range(2, 20)
    buy_change_list=[ c/1000.0 for c in range(-30, -5)]

    result=[]
    for keep_stock, buy_change in product(keep_stock_list, buy_change_list):
        result.append(calc(s1, keep_stock, buy_change))

    print(sorted(result, key=lambda x: x[0], reverse=True))
```

进场:
- 趋势判定: 道氏理论，均线，MACD
  - MACD判断趋势: DIF再0轴上方(看多)，DIF高低点有序向上排列(看多)
- 回调判定: MACD出绿柱
- 回调结束，发现做单区间(是不是可以买)：上涨之后出现回调，但是级别低于趋势(也就是整体上涨，回调少量的时候，就做单)
  - 低周期向上突破

出场:
- 止盈(主动出场): 止盈肯定不是最高点；可以设置固定的止盈点20%, 或者根据行情来判断
- 止损(被动出场): 设置固定的止损点，或者保本，或者保证盈利

三种均线方式
1. $\frac{p_1+p_2+...p_n}{n}>\frac{p_2+p_3+...p_{n+1}}{n}\rightarrow p_1>p_{n+1}$, 涨跌取决于对比$p_1$ 和 $p_{n+1}$
2. 价格突破均线 $p_1 > \frac{p_1+p_2+...p_n}{n} \rightarrow p_1>\frac{p_2+...p_n}{n-1}$, 跌取决于对比$p_1$ 和 往日的平均值(价格在均线上面)
3. 金叉死叉(其中m>n) $\frac{p_1+p_2+...p_n}{n} > \frac{p_1+p_2+...p_n+...p_m}{m} \rightarrow \frac{p_1+p_2+...p_n}{n} > \frac{p_{n+1}+p_{n+2}+...p_m}{m-n}$, 跌取决于对比两个时间段的平均价格

MACD的零点出现早于金叉死叉，但是MACD的零点出现频率高于均线的金叉死叉，所以要结合MACD+均线来判断

背离
- 对于上涨行情：短期均线>长期均线，但是出现了下跌
- 对于下跌行情：短期均线<长期均线，但是出现了上涨

所以一般的策略：
- MACD处于零点，并且出现背离，那么就是买入卖出机会

## Max-Dropdown

```py
import numpy as np
import matplotlib.pyplot as plt

np.random.seed(100)

arr=np.random.random_integers(-1, 1, size=200).cumsum()+10
arr=np.random.random_integers(-1, 1, size=200).cumsum()+50


def get_max_dropdown(arr):
    tmp_max=0
    max_dropdown=0

    final_max_index=0
    final_min_index=0

    for i, a in enumerate(arr):
        if a > tmp_max:
            tmp_max=a
            final_max_index=i
        else:

            drop_down= 1- a/tmp_max
            if drop_down > max_dropdown:
                max_dropdown=drop_down
                final_min_index=i
                peak=final_max_index
    
    return peak, final_min_index, max_dropdown

final_max_index, final_min_index, max_dropdown=get_max_dropdown(arr)
print(final_max_index, final_min_index, max_dropdown)
# 120, 150
# 11, 29

print(1-arr[29]/arr[11])
print(1-arr[150]/arr[120])

plt.plot(np.arange(200), arr, 'r', final_max_index, arr[final_max_index], 'bo', final_min_index, arr[final_min_index], 'go')
plt.show()
```
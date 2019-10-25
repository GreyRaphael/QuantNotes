# Quantitative Trading Basic

Example: basic

```go
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

```go
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
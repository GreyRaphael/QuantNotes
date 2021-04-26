# Derivative

- [Derivative](#derivative)
  - [Call Option & Put Option](#call-option--put-option)


## Call Option & Put Option

看涨期权(Call Option)：在**expiry date**以**specified price**来**买入**某种资产(Underlying Asset)
> 低于strike price, 该期权worthless


看跌期权(Put Option): 在**expiry date**以**specified price**来**卖出**某种资产(Underlying Asset)
> 高于strike price, 该期权worthless

European Option & American Option
- European Option: 必须在expiry data那个时间点
- American Option: 可以在expiry date那个时间点及以前, 但是很少情况提前执行交割

买入看涨期权(long Call Option, 对方交割Share)
> <img src="img/Long-Call.jpg" width="500px"/>
卖出看涨期权(short Call Option, 本方交割Share): 赌不超过strike price, 期权worthless, 挣期权费(Option Premium, Option Fee)
> 一般要求手里有可交割的实物(Covered Call)  
> 如果没有可交割的实物(Uncovered Call), 需要从市场上买入实物，然后交割，亏损可以无穷大  
> <img src="img/Short-Call.jpg" width="500px"/>


买入看跌期权(long Put Option, 对方交割Share)
> <img src="img/Long-Put.jpg" width="500px"/>

卖出看跌期权(short Put Option, 本方交割Share): 赌超过strike price, 期权worthless,为了挣期权费(Option Premium)
> <img src="img/Short-Put.jpg" width="500px"/>
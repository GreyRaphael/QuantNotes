# Plot

- [Plot](#plot)
  - [plotly](#plotly)
  - [altair](#altair)

## plotly

> `pip install plotly`

```py
from vega_datasets import data
import plotly.graph_objects as go

source = data.ohlc()
fig = go.Figure(data=[go.Candlestick(x=source["date"], open=source["open"], high=source["high"], low=source["low"], close=source["close"])])

fig.show()
```

## altair

> `pip install altair`

```py
import altair as alt
from vega_datasets import data

# open in browser
alt.renderers.enable("browser")

# read sample dataframe
source = data.ohlc()
open_close_color = alt.condition("datum.close>datum.open", alt.value("red"), alt.value("green"))
base = alt.Chart(source).encode(
    alt.X("date:T").axis(format="%Y-%m-%d", labelAngle=-45).title("Date"),
    color=open_close_color,
    tooltip=["date", "open", "high", "low", "close"],
)

rule = base.mark_rule().encode(
    alt.Y("low").title("Price"),
    alt.Y2("high"),
)

bar = base.mark_bar().encode(
    alt.Y("open"),
    alt.Y2("close"),
)

candel = (rule + bar).properties(width=1000).configure_scale(zero=False).interactive()
candel.show()
```

completed code

```py
import altair as alt
from vega_datasets import data

# open in browser
alt.renderers.enable("browser")

# read sample dataframe
source = data.ohlc()
open_close_color = alt.condition("datum.close>datum.open", alt.value("red"), alt.value("green"))
base = alt.Chart(source).encode(
    alt.X("date:T").axis(format="%Y-%m-%d", labelAngle=-45).title("Date"),
    color=open_close_color,
    tooltip=["date", "open", "high", "low", "close", "signal"],
)

rule = base.mark_rule().encode(
    alt.Y("low").title("Price"),
    alt.Y2("high"),
)

bar = base.mark_bar(
    fillOpacity=0,  # Make the bar hollow
    strokeWidth=1.5,  # Define the stroke width
).encode(
    y="open",
    y2="close",
    stroke=open_close_color,
)

long_markers = base.transform_filter(alt.datum.signal == "long").mark_point(shape="triangle-up", color="blue", size=80).encode(y="low")
short_markers = base.transform_filter(alt.datum.signal == "short").mark_point(shape="triangle-down", color="yellow", size=80).encode(y="high")

candles = (rule + bar + long_markers + short_markers).properties(width=1000).configure_scale(zero=False).interactive()
candles.show()
```
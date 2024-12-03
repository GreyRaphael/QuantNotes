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

long_markers = (
    base.transform_filter(alt.datum.signal == "long")
    .mark_point(shape="triangle-up", size=80)
    .encode(
        y="low",
        color=alt.value("blue"),  # Override inherited color
        yOffset=alt.value(10),
    )
)
short_markers = (
    base.transform_filter(alt.datum.signal == "short")
    .mark_point(shape="triangle-down", size=80)
    .encode(
        y="high",
        color=alt.value("brown"),  # Override inherited color
        yOffset=alt.value(-10),
    )
)

long_text = (
    base.transform_filter(alt.datum.signal == "long")
    .mark_text(
        align="left",
        baseline="top",
        angle=45,
        yOffset=20,
    )
    .encode(
        y="low",
        color=alt.value("blue"),
        text=alt.Text("signal"),
    )
)

short_text = (
    base.transform_filter(alt.datum.signal == "short")
    .mark_text(
        align="right",
        baseline="bottom",
        angle=60,
        yOffset=-20,
    )
    .encode(
        y="high",
        color=alt.value("brown"),
        text=alt.Text("signal"),
    )
)

# zero: not base on 0
# continuousPadding: add padding
candles = (rule + bar + long_markers + long_text + short_markers + short_text).properties(width=1200).configure_scale(zero=False, continuousPadding=50).interactive()
candles.show()
```

fix text overlapping

```py
import altair as alt
import polars as pl

df = pl.DataFrame(
    {
        "x": [1, 2, 2, 3, 4, 5, 6, 6, 6],
        "y": [4, 8, 8, 9, 11, 12, 9, 9, 9],
        "id": [i for i in range(9)],
    }
)

alt.Chart(df).transform_window(
    cumulative_count="count()",
    groupby=["x", "y"],
).mark_text(dy=alt.expr("20 * datum.cumulative_count")).encode(x="x", y="y", text="y")
```
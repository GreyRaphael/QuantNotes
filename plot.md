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
open_close_color = alt.when("datum.close > datum.open").then(alt.value("red")).otherwise(alt.value("green"))
base = alt.Chart(source).encode(
    alt.X("date:T").axis(format="%Y-%m-%d", labelAngle=-45).title("Date"),
    color=open_close_color,
    tooltip=[
        alt.Tooltip("date:T", title="Date"),
        alt.Tooltip("open:Q", title="Open"),
        alt.Tooltip("high:Q", title="High"),
        alt.Tooltip("low:Q", title="Low"),
        alt.Tooltip("close:Q", title="Close"),
    ],
)
# can zoom in and out
base = base.interactive()

rule = base.mark_rule().encode(
    alt.Y("low:Q").title("Price").scale(zero=False),
    alt.Y2("high:Q"),
)

bar = base.mark_bar().encode(
    alt.Y("open:Q"),
    alt.Y2("close:Q"),
)

candel = (rule + bar).properties(width=1000).configure_scale(zero=False)
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
open_close_color = alt.when("datum.close > datum.open").then(alt.value("red")).otherwise(alt.value("green"))
base = alt.Chart(source).encode(
    alt.X("date:T").axis(format="%Y-%m-%d", labelAngle=-45).title("Date"),
    color=open_close_color,
    tooltip=[
        alt.Tooltip("date:T", title="Date"),
        alt.Tooltip("open:Q", title="Open"),
        alt.Tooltip("high:Q", title="High"),
        alt.Tooltip("low:Q", title="Low"),
        alt.Tooltip("close:Q", title="Close"),
    ],
)
# can zoom in and out
base = base.interactive()

rule = base.mark_rule().encode(
    alt.Y("low:Q").title("Price").scale(zero=False),
    alt.Y2("high:Q"),
)


bar = base.mark_bar(
    fillOpacity=0,  # Make the bar hollow
    strokeWidth=1.5,  # Define the stroke width
).encode(
    y="open:Q",
    y2="close:Q",
    stroke=open_close_color,
)

candel = (rule + bar).properties(width=1000).configure_scale(zero=False)
candel.show()
```
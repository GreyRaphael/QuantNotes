# Strategy

- [Strategy](#strategy)
  - [Turtle](#turtle)

## Turtle

The **True Range** is a measure of market volatility

$$
\text{TR}_t = \max \left\{
\begin{aligned}
& \text{High}_t - \text{Low}_t, \\
& \left| \text{High}_t - \text{Close}_{t-1} \right|, \\
& \left| \text{Low}_t - \text{Close}_{t-1} \right|
\end{aligned}
\right\}
$$

The **Average True Range** is calculated as the moving average of the True Range over a specified period $N$ (e.g., 20 days):

$$
\text{ATR}_t = \frac{1}{N} \sum_{i=t-N+1}^{t} \text{TR}_i
$$

**Unit Size** determines how many shares or contracts to trade and is calculated based on the risk per trade and ATR:

$$
\text{Unit Size} = \frac{\text{Risk Amount}}{\text{ATR}_t \times \text{ATR Multiplier}}
$$

Where:

- **Risk Amount** :
  $$
  \text{Risk Amount} = \text{Equity} \times \text{Risk per Trade (\%)}
  $$

- **ATR Multiplier** = Constant (e.g., 2).

The stop-loss price is set at a distance from the entry price based on the ATR.

- **For Long Positions**:

  $$
  \text{Stop-Loss}_{\text{Long}} = \text{Entry Price} - (\text{ATR Multiplier} \times \text{ATR}_t)
  $$

- **For Short Positions**:

  $$
  \text{Stop-Loss}_{\text{Short}} = \text{Entry Price} + (\text{ATR Multiplier} \times \text{ATR}_t)
  $$

Entry Signals

- **Long Entry**: Enter a long position when the current closing price exceeds the highest high over the previous $N$ periods:

  $$
  \text{If } \text{Close}_t > \max \left( \text{High}_{t-N}, \text{High}_{t-N+1}, \dots, \text{High}_{t-1} \right)
  $$

- **Short Entry**: Enter a short position when the current closing price falls below the lowest low over the previous $N$ periods:

  $$
  \text{If } \text{Close}_t < \min \left( \text{Low}_{t-N}, \text{Low}_{t-N+1}, \dots, \text{Low}_{t-1} \right)
  $$

Exit Signals

- **Exit for Long Positions**:

  - **Stop-Loss Exit**: If the current price falls to or below the stop-loss level.

    $$
    \text{If } \text{Close}_t \leq \text{Stop-Loss}_{\text{Long}}
    $$

  - **Exit Breakout**: If the current price falls below the lowest low over the previous $M$ periods:

    $$
    \text{If } \text{Close}_t < \min \left( \text{Low}_{t-M}, \text{Low}_{t-M+1}, \dots, \text{Low}_{t-1} \right)
    $$

- **Exit for Short Positions**:

  - **Stop-Loss Exit**: If the current price rises to or above the stop-loss level.

    $$
    \text{If } \text{Close}_t \geq \text{Stop-Loss}_{\text{Short}}
    $$

  - **Exit Breakout**: If the current price rises above the highest high over the previous $M$ periods:

    $$
    \text{If } \text{Close}_t > \max \left( \text{High}_{t-M}, \text{High}_{t-M+1}, \dots, \text{High}_{t-1} \right)
    $$

Where:

- $M$ = Exit period (e.g., 10 days), typically shorter than the entry period.

Equity and Position Updates

- **Entering a Long Position**:

  $$
  \text{Equity}_{\text{after}} = \text{Equity}_{\text{before}} - (\text{Unit Size} \times \text{Entry Price})
  $$

- **Exiting a Long Position**:

  $$
  \text{Equity}_{\text{after}} = \text{Equity}_{\text{before}} + (\text{Unit Size} \times \text{Exit Price})
  $$

- **Entering a Short Position**:

  $$
  \text{Equity}_{\text{after}} = \text{Equity}_{\text{before}} + (\text{Unit Size} \times \text{Entry Price})
  $$

- **Exiting a Short Position**:

  $$
  \text{Equity}_{\text{after}} = \text{Equity}_{\text{before}} - (\text{Unit Size} \times \text{Exit Price})
  $$
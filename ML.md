# Machine Learning for secrities

- [Machine Learning for secrities](#machine-learning-for-secrities)
  - [tradiational](#tradiational)
  - [gbm](#gbm)
    - [lightGBM](#lightgbm)
    - [perpetual](#perpetual)
  - [Transformer-Based Models](#transformer-based-models)
  - [LSTM and Advanced Variants](#lstm-and-advanced-variants)
  - [Graph Neural Networks](#graph-neural-networks)

## tradiational

- ARIMA/SARIMA Models: Good for univariate time series but may not capture complex patterns.
- Prophet (by Facebook): User-friendly and interpretable but may not outperform LightGBM in accuracy.
- LSTM/Transformer Models: Could be considered if you have the capacity to gather more data. Risk of overfitting with limited data.

## gbm

### lightGBM
lightGBM features:
- efficiency and high performance on smaller datasets.
- LightGBM can handle time series forecasting by incorporating **lag** features and **rolling** statistics.
- provides insights into feature importance
- Less Risk of Overfitting

Implementation tips:
- Feature Engineering
  - Create lag features (e.g., previous day's close price).
  - Include rolling statistics (e.g., moving averages).
  - Encode the date feature to capture temporal patterns (e.g., day of the week, month).

### perpetual

## Transformer-Based Models

Temporal Fusion Transformers

## LSTM and Advanced Variants

- LSTM
- ALSTM
- xLSTM

## Graph Neural Networks

- Inter-Stock Relationships: If you have information on how stocks are related (e.g., industry sectors, supply chains), GAT can model these connections.
- Dynamic Graphs: Can handle changing relationships over time if the connections between stocks are not static.
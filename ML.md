# Machine Learning for secrities

- [Machine Learning for secrities](#machine-learning-for-secrities)
  - [AutoML](#automl)
  - [tradiational](#tradiational)
  - [gbm](#gbm)
    - [lightGBM](#lightgbm)
    - [perpetual](#perpetual)
  - [Transformer-Based Models](#transformer-based-models)
  - [LSTM and Advanced Variants](#lstm-and-advanced-variants)
  - [Graph Neural Networks](#graph-neural-networks)

## AutoML

AutoML [frameworks](https://openml.github.io/automlbenchmark/frameworks.html)
- Flaml(4k stars, **recommended**): support xgb,lgb,torch; easy2use
- AutoGluon(8k stars, **recommended**): support xgb,lgb,torch; too hard to install
- TPOT or TPOT2.0(9.7k stars): can **exported_pipeline**, only support xgboost, not lightboost or catboost
- auto-sklearn(7.6k stars): not support xgb, lgb just sklearn
- mljar-supervised(3k stars): too many parameters
- h2o-3(6.9k stars): hard to use

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

hybrid model:
> For small dataset, One such approach is the LSTM-LightGBM hybrid model. In this framework, an LSTM network is first used to capture temporal dependencies and generate features, which are then fed into a LightGBM model for final prediction. This method leverages the sequence modeling capabilities of LSTM and the efficiency of LightGBM.

Transfer Learning in Stock Prediction:
> A study titled "A Novel Approach to Short-Term Stock Price Movement Prediction using Transfer Learning" demonstrated that transfer learning could enhance prediction accuracy. The researchers pre-trained a model on multiple stocks and fine-tuned it on a target stock, resulting in improved performance compared to models trained solely on the target stock. 

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
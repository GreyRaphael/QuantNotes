# Machine Learning for secrities

- [Machine Learning for secrities](#machine-learning-for-secrities)
  - [AutoML](#automl)
    - [perpetual gbm automl](#perpetual-gbm-automl)
    - [flaml gbm automl](#flaml-gbm-automl)
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


### perpetual gbm automl

`pip install perpetual`

```py
import perpetual
from flaml.automl.data import load_openml_dataset
from sklearn.metrics import r2_score

X_train, X_test, y_train, y_test = load_openml_dataset(dataset_id=537, data_dir="./")

model = perpetual.PerpetualBooster(objective="SquaredLoss")
model.fit(X_train, y_train, budget=0.8)  # only tuning this budget: [0.0, 1.0]
y_trained_pred = model.predict(X_train)
print(f"Train r2={r2_score(y_train, y_trained_pred)}")

y_pred = model.predict(X_test)
print(f"Test r2={r2_score(y_test, y_pred)}")
print(model.calculate_feature_importance())
```

### flaml gbm automl

`pip install "flaml[automl]"`
> very easy: following the guide: https://microsoft.github.io/FLAML/docs/Examples/AutoML-Regression

```py
from flaml import AutoML
from flaml.automl.data import load_openml_dataset
from sklearn.metrics import r2_score

# Initialize an AutoML instance
automl = AutoML()
# Specify automl goal and constraint
automl_settings = {
    "time_budget": 30,  # in seconds, tuning this
    "metric": "r2",
    "task": "regression",
    "estimator_list": ["lgbm", "xgboost"],
    "log_file_name": "california.log",
    "seed": 7654321,  # random seed
}

# X_train shape: (15480, 8), y_train shape: (15480,)
# X_test shape: (5160, 8), y_test shape: (5160,)
# data from: https://www.openml.org/d/537
X_train, X_test, y_train, y_test = load_openml_dataset(dataset_id=537, data_dir="./")

# Train with labeled input data
automl.fit(X_train=X_train, y_train=y_train, **automl_settings)
# Print the best model
print(automl.model.estimator)
print(f"Best r2 on validation data: {1 - automl.best_loss}")
# Predict
y_pred = automl.predict(X_test)
print(f"Test r2: {r2_score(y_test, y_pred)}")

print(f"feature_names: {automl.feature_names_in_}")
print(f"importances: {automl.feature_importances_}")
```

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
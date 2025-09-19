# Machine Learning for secrities

- [Machine Learning for secrities](#machine-learning-for-secrities)
  - [AutoML](#automl)
    - [perpetual gbm automl](#perpetual-gbm-automl)
      - [perpetual in rust](#perpetual-in-rust)
    - [flaml gbm automl](#flaml-gbm-automl)
  - [tradiational](#tradiational)
  - [gbm](#gbm)
    - [lightGBM](#lightgbm)
    - [perpetual](#perpetual)
  - [Transformer-Based Models](#transformer-based-models)
  - [LSTM and Advanced Variants](#lstm-and-advanced-variants)
  - [Graph Neural Networks](#graph-neural-networks)
  - [Walk-forward Cross-validation in ranking fold split](#walk-forward-cross-validation-in-ranking-fold-split)

## AutoML

> Acceptable R² values can vary by field. For example, in some social sciences, an R² of 0.3 might be considered good, while in engineering, you might aim for 0.9 or higher.

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

#### perpetual in rust

best practice: use python to train the dataset and save model, load model in rust and predict

> rust must be nightly version  
> `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`  
> `rustup default beta`

How to use: `cargo add perpetual`

```rs
use perpetual::{Matrix, PerpetualBooster};

fn main() {
    let model = PerpetualBooster::load_booster("my.model").expect("load eror");
    let record = vec![-121.24, 39.37, 16.0, 2785.0, 616.0, 1387.0, 530.0, 2.3886];
    let input = Matrix::new(&record, 1, 8);
    let result = model.predict(&input, false);
    println!("prediction={:?}", result);
}
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

## Walk-forward Cross-validation in ranking fold split

`from sklearn.model_selection import TimeSeriesSplit`不支持group

`from mlxtend.evaluate.time_series import GroupTimeSeriesSplit`支持group

```py
# Walk-forward Cross-validation
from sklearn.model_selection import TimeSeriesSplit
from mlxtend.evaluate.time_series import GroupTimeSeriesSplit
import numpy as np
import pandas as pd
import catboost
import xgboost as xgb

n_groups = 28963  # 5 个 query
samples_per_group = 6  # 每个 query 有 6 个样本
total_samples = n_groups * samples_per_group

df = pd.DataFrame(
    {
        "group_id": np.repeat(np.arange(n_groups), samples_per_group),
        "time": np.tile(np.arange(samples_per_group), n_groups),
        "feature1": np.random.randn(total_samples),
        "feature2": np.random.randn(total_samples),
        "label": np.random.randint(0, 5, size=total_samples),  # relevance
    }
)

# tscv = TimeSeriesSplit(max_train_size=3, n_splits=3)
tscv = GroupTimeSeriesSplit(test_size=350 * 16, n_splits=3, shift_size=250 * 16)

for i, (train_index, test_index) in enumerate(tscv.split(df["label"], groups=df['group_id'])):
    print(f"Fold {i}:")
    print(f"  Train: index={train_index}, size={train_index.size}")
    print(f"  Test:  index={test_index}, size={test_index.size}")

pool = catboost.Pool(data=df[["time", "feature1", "feature2"]], label=df["label"], group_id=df["group_id"])

cat_gts = GroupTimeSeriesSplit(test_size=350 * 16, n_splits=3, shift_size=250 * 16)
for i, (train_index, test_index) in enumerate(cat_gts.split(pool.get_label(), groups=pool.get_group_id_hash())):
    print(f"Fold {i}:")
    print(f"  Train: index={train_index}, size={train_index.size}")
    print(f"  Test:  index={test_index}, size={test_index.size}")

dtrain = xgb.DMatrix(data=df[["time", "feature1", "feature2"]], label=df["label"], qid=df["group_id"])

xgb_gts = GroupTimeSeriesSplit(test_size=350 * 16, n_splits=4, shift_size=250 * 16)
xgb_gp = dtrain.get_group()
qid = np.repeat(np.arange(xgb_gp.size), repeats=xgb_gp)
for i, (train_index, test_index) in enumerate(xgb_gts.split(dtrain.get_label(), groups=qid)):
    print(f"Fold {i}:")
    print(f"  Train: index={train_index}, size={train_index.size}")
    print(f"  Test:  index={test_index}, size={test_index.size}")
```
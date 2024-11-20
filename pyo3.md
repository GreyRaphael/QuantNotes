# pyo3

- [pyo3](#pyo3)
  - [python override rust trait](#python-override-rust-trait)

How to use pyo3
1. download [maturin](https://github.com/PyO3/maturin/releases)
2. `maturin new proj1`
3. change `Cargo.toml`
4. `maturin develop`

```toml
[package]
name = "bktrader"
version = "0.9.0"
edition = "2021"

# See more keys and their definitions at https://doc.rust-lang.org/cargo/reference/manifest.html
[lib]
name = "bktrader"
crate-type = ["cdylib"]

[dependencies.pyo3]
version = "0.22"
# "abi3-py38" tells pyo3 (and maturin) to build using the stable ABI with minimum Python version 3.8
features = ["abi3-py38"]
```

## python override rust trait

```bash
maturin new proj1
# edit src/lib.rs
```

```rs
// src/lib.rs
use pyo3::prelude::*;
use pyo3::types::PyAny;

#[pyclass]
#[derive(Debug, Copy, Clone)]
pub struct Bar {
    #[pyo3(get)]
    price: f64,
}

#[pymethods]
impl Bar {
    #[new]
    fn new(price: f64) -> Self {
        Bar { price }
    }
}

pub trait Strategy {
    fn on_bar(&self, bar: &Bar);
}

#[pyclass]
struct StrategyWrapper {
    inner: Py<PyAny>,
}

#[pymethods]
impl StrategyWrapper {
    #[new]
    fn new(inner: Py<PyAny>) -> Self {
        StrategyWrapper { inner }
    }

    fn execute(&self, bar: &mut Bar) {
        for i in 0..3 {
            bar.price += i as f64 * 100.0;
            self.on_bar(bar);
        }
    }
}

impl Strategy for StrategyWrapper {
    fn on_bar(&self, bar: &Bar) {
        Python::with_gil(|py| {
            if let Err(e) = self.inner.call_method1(py, "on_bar", (bar.into_py(py),)) {
                eprintln!("Error calling Python method: {}", e);
            }
        });
    }
}

/// A Python module implemented in Rust.
#[pymodule]
fn py(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_class::<Bar>()?;
    m.add_class::<StrategyWrapper>()?;
    Ok(())
}
```

```py
# test.py
import proj1

print(proj1.__all__)

class MyStrategy:
    def on_bar(self, bar):
        print(f"Bar price: {bar.price}")

obj=MyStrategy()
wrapper=proj1.StrategyWrapper(obj)
bar=proj1.Bar(100)

wrapper.execute(bar)
```
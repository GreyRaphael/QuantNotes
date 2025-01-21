# Rust with Quant

- [Rust with Quant](#rust-with-quant)
  - [SIMD](#simd)
  - [Factor calc](#factor-calc)

## SIMD

use SIMD in rust nightly without any dependencies

```rs
#![feature(portable_simd)]

use std::simd::{num::SimdFloat, Simd};

fn main() {
    // This example uses 4-wide f32 vectors.
    // If you need a different width, you can specify Simd<f32, LANES> with
    // a matching const generic for the number of lanes.
    let v = vec![1.0, 2.0, 3.0, 4.0];
    let a = Simd::<f64, 4>::from_slice(&v);
    let b = Simd::<f64, 4>::from_array([1.0, 2.0, 3.0, 4.0]);

    // The dot product is computed by multiplying pairwise and then summing the lanes.
    let dot_product = (a * b).reduce_sum();

    println!("dot(a, b) = {}", dot_product);
}
```

use SIMD for arbitratry length vectors
> `reduce_sum` is heavy, avoids doing a horizontal sum on every chunk.

```rs
// good
#![feature(portable_simd)]
use std::simd::{f64x4, num::SimdFloat};

/// Compute the dot product of two arbitrary-length slices using 4-wide f64x4 SIMD.
fn dot_product(a: &[f64], b: &[f64]) -> f64 {
    // Ensure we don't go out of bounds if lengths differ.
    let n = a.len().min(b.len());

    // Accumulate partial sums in a SIMD register.
    let mut simd_sum = f64x4::splat(0.0);

    // Process chunks of 4 at a time.
    let mut i = 0;
    while i + 4 <= n {
        // Load the next chunk of a and b into SIMD registers.
        let va = f64x4::from_slice(&a[i..i + 4]);
        let vb = f64x4::from_slice(&b[i..i + 4]);

        // Multiply pairwise, then add to the running sum.
        simd_sum += va * vb;

        i += 4;
    }

    // Sum the lanes of `simd_sum` into a single f64.
    // reduce_sum is heavy, don't put it into the above loop
    let mut sum = simd_sum.reduce_sum();

    // Process any leftover elements if the length isn't a multiple of 4.
    while i < n {
        sum += a[i] * b[i];
        i += 1;
    }

    sum
}

fn main() {
    let x = vec![
        1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0, 3.0,
        4.0, 5.0,
    ];
    let y = vec![
        1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0, 3.0, 4.0, 5.0, 1.0, 2.0, 3.0,
        4.0, 5.0,
    ];
    let dot = dot_product(&x, &y);
    println!("dot = {dot}");
}
```

```rs
// less performant
#![feature(portable_simd)]
use std::simd::{f64x4, num::SimdFloat};

fn dot_product(a: &[f64], b: &[f64]) -> f64 {
    let n = a.len().min(b.len());
    let mut sum = 0.0;

    let mut i = 0;
    while i + 4 <= n {
        let va = f64x4::from_slice(&a[i..i + 4]);
        let vb = f64x4::from_slice(&b[i..i + 4]);
        sum += (va * vb).reduce_sum();

        i += 4;
    }

    // Process any leftover elements if the length isn't a multiple of 4.
    while i < n {
        sum += a[i] * b[i];
        i += 1;
    }

    sum
}
```

## Factor calc

```rs
use std::fmt;

#[derive(Default)]
struct SixChars([u8; 6]); // char is 4bytes, so choose u8

impl From<&str> for SixChars {
    fn from(value: &str) -> Self {
        let mut array = [0u8; 6];
        let bytes = value.as_bytes();
        array.copy_from_slice(&bytes[0..6]);
        SixChars(array)
    }
}

impl fmt::Display for SixChars {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        for &byte in &self.0 {
            // Convert each byte to a char and write it to the formatter
            write!(f, "{}", byte as char)?;
        }
        Ok(())
    }
}

impl fmt::Debug for SixChars {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // Attempt to convert the array of u8 to a string slice
        let string_repr = std::str::from_utf8(&self.0).map_err(|_| fmt::Error)?;

        // Write formatted output with double quotes
        write!(f, "SixChars(\"{}\")", string_repr)
    }
}

#[derive(Debug, Default)]
struct TickData {
    symbol: SixChars,
    // symbol: CompactString,
    open: u32,
    high: u32,
    low: u32,
    last: u32,
    volume: u64,
    amount: f64,
    ask_prices: [u32; 10],
    bid_prices: [u32; 10],
    ask_volumes: [u32; 10],
    bid_volumes: [u32; 10],
}

#[derive(Debug, Default)]
struct BarData {
    symbol: SixChars,
    // symbol: CompactString,
    open: u32,
    high: u32,
    low: u32,
    last: u32,
    volume: u64,
    amount: f64,
}

// Step 1: Define the Trait
trait IFactor<T> {
    fn update(&mut self, quote: &T);
}

// Step 2: Define the Structs
#[derive(Debug, Default)]
struct Factor01 {
    factor: f64,
    name: String,
}

#[derive(Debug, Default)]
struct Factor02 {
    factor: f64,
    name: String,
}

impl IFactor<TickData> for Factor01 {
    fn update(&mut self, quote: &TickData) {
        self.factor = (quote.open + quote.last) as f64 / 2.0;
    }
}

impl IFactor<BarData> for Factor01 {
    fn update(&mut self, quote: &BarData) {
        self.factor = (quote.open + quote.last) as f64 / 2.0;
    }
}

impl IFactor<TickData> for Factor02 {
    fn update(&mut self, quote: &TickData) {
        self.factor = quote.amount / (quote.volume as f64);
    }
}

// Step 3: Define the Enum
enum GwFactors {
    A(Factor01),
    B(Factor02),
}

impl IFactor<TickData> for GwFactors {
    fn update(&mut self, quote: &TickData) {
        println!("update tick");
        match self {
            GwFactors::A(a) => a.update(quote),
            GwFactors::B(b) => b.update(quote),
        }
    }
}

impl IFactor<BarData> for GwFactors {
    fn update(&mut self, quote: &BarData) {
        println!("update bar {:?}", quote);
    }
}

impl GwFactors {
    fn get_factor(&self) -> f64 {
        match self {
            GwFactors::A(a) => a.factor,
            GwFactors::B(b) => b.factor,
        }
    }
    fn get_name(&self) -> &str {
        match self {
            GwFactors::A(a) => &a.name,
            GwFactors::B(b) => &b.name,
        }
    }
}

// Step 5: Store in a Vec
fn main() {
    let mut vec = vec![
        GwFactors::A(Factor01 {
            name: "factor01".into(),
            ..Default::default()
        }),
        GwFactors::B(Factor02 {
            name: "factor02".into(),
            ..Default::default()
        }),
    ];

    let quote = TickData {
        symbol: "688009".into(),
        open: 100,
        last: 200,
        volume: 100000,
        amount: 6597421.0,
        ..Default::default()
    };

    for item in &mut vec {
        item.update(&quote);
    }

    for item in &vec {
        println!("name={}, factor={}", item.get_name(), item.get_factor())
    }

    let bar=BarData{
        symbol: "300116".into(),
        open:1000,
        high:2000,
        ..Default::default()
    };

    vec[0].update(&bar);
    vec[0].update(&quote);
}

// fn str_to_char_array(s: &str) -> [char; 6] {
//     let mut chars = [char::default(); 6];
//     for (i, c) in s.chars().enumerate() {
//         chars[i] = c;
//     }
//     chars
// }

// fn char_array_to_string(arr: [char; 6]) -> String {
//     arr.iter().collect()
// }
```
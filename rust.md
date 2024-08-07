# Rust with Quant

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
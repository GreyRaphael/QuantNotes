# arrow

- [arrow](#arrow)
  - [installation](#installation)
  - [Simple Usage](#simple-usage)
  - [pyarrow](#pyarrow)
    - [RecordBatch vs Table](#recordbatch-vs-table)
    - [compute functions](#compute-functions)

## installation

`vcpkg install arrow`

## Simple Usage

[DataTypes](https://arrow.apache.org/docs/python/api/datatypes.html)

```cpp
#include <arrow/api.h>
#include <arrow/array/array_binary.h>
#include <arrow/array/array_nested.h>
#include <arrow/io/api.h>
#include <arrow/io/type_fwd.h>
#include <arrow/ipc/api.h>
#include <arrow/record_batch.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <vector>

#include "DataTypes.h"

arrow::Result<std::vector<TickData>> RunRowConversion(std::shared_ptr<arrow::RecordBatch> const& rb) {
    SPDLOG_INFO("BEGIN CONVERSION: schema:\n{}", rb->schema()->ToString());
    std::vector<std::shared_ptr<arrow::Field>> schema_vec{
        arrow::field("date", arrow::timestamp(arrow::TimeUnit::NANO)),
        arrow::field("instrument", arrow::uint32()),
        arrow::field("pre_close", arrow::float32()),
        arrow::field("open", arrow::float32()),
        arrow::field("price", arrow::float32()),
        arrow::field("num_trades", arrow::int64()),
        arrow::field("volume", arrow::int64()),
        arrow::field("total_ask_volume", arrow::int64()),
        arrow::field("total_bid_volume", arrow::int64()),
        arrow::field("amount", arrow::float64()),
        arrow::field("max_ask_one", arrow::int32()),
        arrow::field("max_bid_one", arrow::int32()),
        arrow::field("ask_volumes", arrow::list(arrow::int32())),

    };
    auto expected_schema = std::make_shared<arrow::Schema>(schema_vec);
    if (!expected_schema->Equals(rb->schema())) {
        return arrow::Status::Invalid("Schemas are not matching!");
    }

    auto timestamp_arr = std::static_pointer_cast<arrow::TimestampArray>(rb->GetColumnByName("date"));
    auto secucode_arr = std::static_pointer_cast<arrow::UInt32Array>(rb->GetColumnByName("instrument"));
    auto preclose_arr = std::static_pointer_cast<arrow::FloatArray>(rb->GetColumnByName("pre_close"));
    auto open_arr = std::static_pointer_cast<arrow::FloatArray>(rb->GetColumnByName("open"));
    auto last_arr = std::static_pointer_cast<arrow::FloatArray>(rb->GetColumnByName("price"));
    auto num_trades_arr = std::static_pointer_cast<arrow::Int64Array>(rb->GetColumnByName("num_trades"));
    auto volume_arr = std::static_pointer_cast<arrow::Int64Array>(rb->GetColumnByName("volume"));
    auto total_ask_volume_arr = std::static_pointer_cast<arrow::Int64Array>(rb->GetColumnByName("total_ask_volume"));
    auto total_bid_volume_arr = std::static_pointer_cast<arrow::Int64Array>(rb->GetColumnByName("total_bid_volume"));
    auto amount_arr = std::static_pointer_cast<arrow::DoubleArray>(rb->GetColumnByName("amount"));
    auto ask_one_orders_arr = std::static_pointer_cast<arrow::Int32Array>(rb->GetColumnByName("max_ask_one"));
    auto bid_one_orders_arr = std::static_pointer_cast<arrow::Int32Array>(rb->GetColumnByName("max_bid_one"));
    auto ask_vols = std::static_pointer_cast<arrow::FixedSizeListArray>(rb->GetColumnByName("ask_volumes"));

    std::vector<TickData> datas{};
    datas.reserve(rb->num_rows());
    for (size_t i = 0; i < rb->num_rows(); ++i) {
        TickData tick{};
        tick.timestamp = timestamp_arr->Value(i);
        tick.secucode = std::format("{:06d}", secucode_arr->Value(i));
        tick.preclose = preclose_arr->Value(i);
        tick.open = open_arr->Value(i);
        tick.last = last_arr->Value(i);
        tick.num_trades = num_trades_arr->Value(i);
        tick.volume = volume_arr->Value(i);
        tick.total_ask_volume = total_ask_volume_arr->Value(i);
        tick.total_bid_volume = total_bid_volume_arr->Value(i);
        tick.amount = amount_arr->Value(i);
        tick.ask_one_orders[0] = ask_one_orders_arr->Value(i);
        tick.bid_one_orders[0] = bid_one_orders_arr->Value(i);

        auto ask_vols_ptr = std::static_pointer_cast<arrow::Int64Array>(ask_vols->value_slice(i));
        for (int j = 0; j < 10; ++j) {
            tick.ask_volumes[j] = ask_vols_ptr->Value(j);
        }

        datas.emplace_back(tick);
    }

    return datas;
}

arrow::Status ReadFromFile(std::string const& filename) {
    ARROW_ASSIGN_OR_RAISE(auto infile, arrow::io::ReadableFile::Open(filename, arrow::default_memory_pool()));
    ARROW_ASSIGN_OR_RAISE(auto reader, arrow::ipc::RecordBatchFileReader::Open(infile))
    // only 1 record batch
    auto rb = reader->ReadRecordBatch(0).MoveValueUnsafe();
    auto result = RunRowConversion(rb);

    int64_t count = 0;
    for (auto&& e : *result) {
        auto tp = std::chrono::system_clock::time_point(std::chrono::nanoseconds(e.timestamp));
        auto line = std::format("{:%Y-%m-%d %H:%M:%OS},code={},last={},vol={},amount={},ask_vols=", tp, e.secucode, e.last, e.volume, e.amount);
        std::cout << line << '[';
        for (size_t i = 0; i < 10; ++i) {
            std::cout << e.ask_volumes[i] << ' ';
        }
        std::cout << "]\n";

        ++count;
    }
    SPDLOG_INFO("total={}", count);

    return arrow::Status::OK();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "provide an ipc file!" << '\n';
        return -1;
    }
    auto status = ReadFromFile(argv[1]);
    if (!status.ok()) {
        std::cerr << status.ToString() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

## pyarrow

[basic cookbook](https://arrow.apache.org/cookbook/py/)

[pyarrow guide&api](https://arrow.apache.org/docs/python/index.html)

### RecordBatch vs Table

```py
import pyarrow as pa

batch = pa.record_batch(
    [
        pa.array([1, 2, 3, 4]),
        pa.array(["foo", "bar", "baz", None]),
        pa.array([True, None, False, True]),
    ],
    names=["f0", "f1", "f2"],
)
# pyarrow.RecordBatch
# f0: int64
# f1: string
# f2: bool
# ----
# f0: [1,2,3,4]
# f1: ["foo","bar","baz",null]
# f2: [true,null,false,true]

col0 = batch.column(0)  # Int64Array
col0_row2 = col0[2]  # Int64Scalar
col0_row2.as_py() # 3

# pa.Table
tb = pa.Table.from_batches([batch for _ in range(3)])

col0 = tb.column(0) # ChunkedArray
col0_row2 = col0[5] # Int64Scalar
col0_row2.as_py() # 1

cks = col0.chunks # list of Int64Array
ck0 = col0.chunk(0) # Int64Array
ck0_row2 = ck0[2] # Int64Scalar
```

### compute functions

[pyarrow.compute.round_to_multiple](https://arrow.apache.org/docs/python/generated/pyarrow.compute.round_to_multiple.html)

```py
import pyarrow as pa
import pyarrow.compute as pc

arr01 = pa.array([1, 2, 3, 4, 5, 6, 7])
print(pc.round_to_multiple(arr01, 3, round_mode="down"))  # [0,0,3,3,3,6,6]
print(pc.round_to_multiple(arr01, 3, round_mode="up"))  # [3,3,3,6,6,6,9]
```

[pyarrow.compute.trunc](https://arrow.apache.org/docs/python/generated/pyarrow.compute.trunc.html)

```py
arr01 = pa.array([1.1, 2.1, 3.1, 4, 5, 6, 7])
pc.trunc(arr01) # [1, 2, 3, 4, 5, 6, 7]
```

[pyarrow.compute.case_when](https://arrow.apache.org/docs/python/generated/pyarrow.compute.case_when.html)

```py
arr01 = pa.array([1, 2, 3, 4, 5])

cond1 = pc.less(arr01, 2)
cond2 = pc.greater(arr01, 4)
conditions = pa.StructArray.from_arrays([cond1, cond2], names=["cond1", "cond2"])
cases = [pa.scalar(0), pa.scalar(10), arr01]

pc.case_when(conditions, *cases) # [0, 2, 3, 4, 10]
```
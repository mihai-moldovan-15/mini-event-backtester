#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <cstdint>
#include <iomanip>

using Timestamp = std::int64_t;//stored in nanoseconds
using SequenceNumber = std::int64_t;//differentiate between buys/sells when modifying an order
using Price = std::int64_t;//price in cents
using Position = std::int64_t;
using Cash = std::int64_t;
using OrderId = std::int64_t;
using FillId = std::int64_t;
using Quantity = std::int32_t;
using Symbol = std::string;
using SymbolView = std::string_view;

enum class Side {
    Buy,
    Sell,
};

enum class OrderType {
    Market,
    Limit,
};

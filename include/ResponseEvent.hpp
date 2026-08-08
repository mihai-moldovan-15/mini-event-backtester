#pragma once
#include "Types.hpp"
#include <optional>

enum class ResponseType {
    Cancelled,
    CancelFailed,
    Modified,
    ModifyFailed,
    Resting,

    Filled,
    PartiallyFilled,
};

struct ResponseEvent {
    ResponseType type;
    OrderId orderId;
    Symbol symbol;
    Timestamp ts{};
    Side side{};
    bool isOwn{};

    std::optional<Quantity> fillQty{};
    std::optional<Price> fillPrice{};
};
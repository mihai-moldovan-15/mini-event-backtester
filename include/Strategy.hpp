#pragma once
#include <vector>
#include "Types.hpp"
#include "Portfolio.hpp"
#include "ResponseEvent.hpp"
#include "OrderBook.hpp"
#include "Fill.hpp"
#include "Event.hpp"
#include <memory>

class Strategy {
public:
    virtual std::vector<std::unique_ptr<Event>> onTime
    (Timestamp ts,
    const std::unordered_map<Symbol, OrderBook>& orderBooks,
    const Portfolio& portfolio,
    const std::vector<ResponseEvent>& eventsSinceLastCall) = 0;

    virtual ~Strategy() = default;
};

#include "OrderBook.hpp"

#include <format>
#include <stdexcept>

void OrderBook::validate() const {
    if (m_symbol.empty())
        throw std::invalid_argument("Missing book symbol");

    if (!m_relevantAsks.empty() &&!m_relevantBids.empty() && m_relevantAsks.begin()->first < m_relevantBids.begin()->first)
        throw std::logic_error("Book cannot be crossed");

    if (!m_relevantBids.empty() && m_relevantBids.rbegin()->first < 0)
        throw std::invalid_argument("Bid prices cannot be negative");

    if (!m_otherBids.empty() && m_otherBids.rbegin()->first < 0)
        throw std::invalid_argument("Bid prices cannot be negative");

    for (const auto& [price, level] : m_relevantBids) {
        if (level.totalQuantity <= 0)
            throw std::logic_error("Bid level has non-positive quantity at " + std::to_string(price));

        Quantity sum{};
        for (auto it = level.orders.getHead(); it != nullptr; it=it->next)
            sum += it->value.getQuantity();

        if (sum != level.totalQuantity)
            throw std::logic_error("Bid level totalQuantity mismatch at price " + std::to_string(price));
    }

    for (const auto& [price, level] : m_otherBids) {
        if (level.totalQuantity <= 0)
            throw std::logic_error("Bid level has non-positive quantity at " + std::to_string(price));

        Quantity sum{};
        for (auto it = level.orders.getHead(); it != nullptr; it=it->next)
            sum += it->value.getQuantity();

        if (sum != level.totalQuantity)
            throw std::logic_error("Bid level totalQuantity mismatch at price " + std::to_string(price));
    }

    if (!m_relevantAsks.empty() && m_relevantAsks.begin()->first < 0)
        throw std::invalid_argument("Ask prices cannot be negative");
    if (!m_otherAsks.empty() && m_otherAsks.begin()->first < 0)
        throw std::invalid_argument("Ask prices cannot be negative");

    for (const auto& [price, level] : m_relevantAsks) {
        if (level.totalQuantity <= 0)
            throw std::logic_error("Ask level has non-positive quantity at " + std::to_string(price));

        Quantity sum{};
        for (auto it = level.orders.getHead(); it != nullptr; it=it->next)
            sum += it->value.getQuantity();

        if (sum != level.totalQuantity)
            throw std::logic_error("Ask level totalQuantity mismatch at price " + std::to_string(price));
    }

    for (const auto& [price, level] : m_otherAsks) {
        if (level.totalQuantity <= 0)
            throw std::logic_error("Ask level has non-positive quantity at " + std::to_string(price));

        Quantity sum{};
        for (auto it = level.orders.getHead(); it != nullptr; it=it->next)
            sum += it->value.getQuantity();

        if (sum != level.totalQuantity)
            throw std::logic_error("Ask level totalQuantity mismatch at price " + std::to_string(price));
    }
}

Price OrderBook::getMarkPrice() const {
    Price bestAskPrice{ getBestAsk() };
    Price bestBidPrice{ getBestBid() };

    if (!bestAskPrice && !bestBidPrice)
        throw std::runtime_error("No available bids or asks for " + m_symbol);

    if (!bestAskPrice)
        return bestBidPrice;

    if (!bestBidPrice)
        return bestAskPrice;

    return (bestBidPrice + bestAskPrice) / 2;
}

Price OrderBook::getSpread() const {
    return getBestAsk() - getBestBid();
}

/// folosim extract - C++ 17
/// extract permite mutarea valorilor dintr un map in altul, fara a fi nevoit sa copiez
/// https://en.cppreference.com/cpp/container/map/extract
void OrderBook::rebalanceLevels() {
    ///promote
    while (!m_otherAsks.empty() && m_relevantAsks.size() < m_relevantLevelCount) {
        auto node_handle = m_otherAsks.extract(m_otherAsks.begin());
        m_relevantAsks.insert(std::move(node_handle));
    }

    while (!m_otherBids.empty() && m_relevantBids.size() < m_relevantLevelCount) {
        auto node_handle = m_otherBids.extract(m_otherBids.begin());
        m_relevantBids.insert(std::move(node_handle));
    }

    ///demote
    while (m_relevantAsks.size() > m_relevantLevelCount) {
        auto node_handle = m_relevantAsks.extract(std::prev(m_relevantAsks.end()));
        m_otherAsks.insert(std::move(node_handle));
    }

    while (m_relevantBids.size() > m_relevantLevelCount) {
        auto node_handle = m_relevantBids.extract(std::prev(m_relevantBids.end()));
        m_otherBids.insert(std::move(node_handle));
    }
}

void OrderBook::recordTrade(const Order& incoming, const Order& existing, Price price, Quantity qty,
                            Timestamp currentTime, std::vector<ResponseEvent>& responses) const {

    const ResponseType incomingType{incoming.getQuantity() - qty == 0 ? ResponseType::Filled : ResponseType::PartiallyFilled};
    const ResponseType existingType{existing.getQuantity() - qty == 0 ? ResponseType::Filled : ResponseType::PartiallyFilled};

    responses.push_back({.type = incomingType, .orderId = incoming.getOrderId(), .symbol = m_symbol,
                              .ts = currentTime, .side = incoming.getOrderSide(),
                                .isOwn = incoming.isOwn(), .fillQty = qty, .fillPrice = price});

    responses.push_back({ .type = existingType, .orderId = existing.getOrderId(), .symbol = m_symbol,
                            .ts = currentTime, .side = existing.getOrderSide(),
                               .isOwn = existing.isOwn(), .fillQty = qty ,.fillPrice = price} );
}

std::vector<ResponseEvent> OrderBook::processAddOrder(const Order& order, Cash availableCash, const Timestamp currentTime) {
    Order newOrder = order;
    std::vector<ResponseEvent> responseEvents{};

    if (newOrder.getOrderSide() == Side::Buy) {
        auto& relevantLevels = m_relevantAsks;
        auto& otherLevels = m_otherAsks;
        bool inRelevant = true;
        auto it = relevantLevels.begin();
        Cash remainingCash = availableCash;
        bool cashExhausted = false;

        while (newOrder.getQuantity() > 0 && !cashExhausted &&
               (inRelevant ? it != relevantLevels.end() : it != otherLevels.end())) {
            const Price existingPrice = it->first;
            if (newOrder.getOrderType() == OrderType::Limit && *newOrder.getLimitPrice() < existingPrice)
                break;                                                                                    ///aici era checkfill ul

            BookLevel& level = it->second;
            auto ordIt = level.orders.getHead();

            while (newOrder.getQuantity() > 0 && ordIt != nullptr && !cashExhausted) {
                Order& existing = ordIt->value;
                Quantity qty = std::min(newOrder.getQuantity(), existing.getQuantity());

                if (newOrder.isOwn()) {
                    auto affordableQty = static_cast<Quantity>(remainingCash / existingPrice);
                    if (affordableQty == 0) {
                        cashExhausted = true;
                        break;
                    }
                    qty = std::min(qty, affordableQty);
                }

                recordTrade(newOrder, existing, existingPrice, qty, currentTime, responseEvents);

                newOrder.setQuantity(newOrder.getQuantity() - qty);
                existing.setQuantity(existing.getQuantity() - qty);
                level.totalQuantity -= qty;

                if (newOrder.isOwn())
                    remainingCash -= qty * existingPrice;

                if (existing.getQuantity() == 0) {
                    m_activeOrders.erase(existing.getOrderId());
                    ordIt = level.orders.erase(ordIt);
                }
                else
                    ordIt = ordIt->next;
            }

            if (inRelevant)
                it = level.orders.empty() ? relevantLevels.erase(it) : std::next(it);
            else
                it = level.orders.empty() ? otherLevels.erase(it) : std::next(it);

            if (inRelevant && it == relevantLevels.end()) {
                inRelevant = false;
                it = otherLevels.begin();
            }
        }
    }
    else { ///Side::Sell
        auto& relevantLevels = m_relevantBids;
        auto& otherLevels = m_otherBids;
        bool inRelevant = true;
        auto it = relevantLevels.begin();
        Cash remainingCash = availableCash;

        while (newOrder.getQuantity() > 0 && it != (inRelevant ? relevantLevels.end() : otherLevels.end())) {
            const Price existingPrice = it->first;
            if (newOrder.getOrderType() == OrderType::Limit && *newOrder.getLimitPrice() > existingPrice)
                break;

            BookLevel& level = it->second;
            auto ordIt = level.orders.getHead();

            while (newOrder.getQuantity() > 0 && ordIt != nullptr) {
                Order& existing = ordIt->value;
                Quantity qty = std::min(newOrder.getQuantity(), existing.getQuantity());

                if (existing.isOwn()) {
                    auto affordableQty = static_cast<Quantity>(remainingCash / existingPrice);
                    qty = std::min(qty, affordableQty);

                    if (qty == 0) {
                        ordIt = ordIt->next;
                        continue;
                    }
                }

                recordTrade(newOrder, existing, existingPrice, qty, currentTime, responseEvents);

                newOrder.setQuantity(newOrder.getQuantity() - qty);
                existing.setQuantity(existing.getQuantity() - qty);
                level.totalQuantity -= qty;

                if (existing.isOwn())
                    remainingCash -= qty * existingPrice;

                if (existing.getQuantity() == 0) {
                    m_activeOrders.erase(existing.getOrderId());
                    ordIt = level.orders.erase(ordIt);
                }
                else
                    ordIt = ordIt->next;
            }

            if (inRelevant)
                it = level.orders.empty() ? relevantLevels.erase(it) : std::next(it);
           else
               it = level.orders.empty() ? otherLevels.erase(it) : std::next(it);

            if (inRelevant && it == relevantLevels.end()) {
                inRelevant = false;
                it = otherLevels.begin();
            }
        }
    }


    if (newOrder.getQuantity() > 0 && newOrder.getOrderType() == OrderType::Limit) {
        if (newOrder.getOrderSide() == Side::Buy) {
            auto [it, _] = m_relevantBids.try_emplace(*newOrder.getLimitPrice(), &m_pool);
            ///BookLevel nu are default ctor, nu putem folosi m_relevantBids[]
            BookLevel& level = it->second;
            level.orders.push_back(newOrder);
            level.totalQuantity += newOrder.getQuantity();
            const auto newOrd = level.orders.getTail();
            m_activeOrders[newOrder.getOrderId()] = OrderLocation{.side = Side::Buy, .price = *newOrder.getLimitPrice(), .level = &level, .it = newOrd};
        }
        else {
            auto [it, _] = m_relevantAsks.try_emplace(*newOrder.getLimitPrice(), &m_pool);
            ///BookLevel nu are default ctor, nu putem folosi m_relevantAsks[]
            BookLevel& level = it->second;
            level.orders.push_back(newOrder);
            level.totalQuantity += newOrder.getQuantity();
            const auto newOrd = level.orders.getTail();
            m_activeOrders[newOrder.getOrderId()] = OrderLocation{.side = Side::Sell, .price = *newOrder.getLimitPrice(), .level = &level, .it = newOrd};
        }

        responseEvents.push_back({.type = ResponseType::Resting, .orderId = newOrder.getOrderId(), .symbol = m_symbol, .ts = currentTime,
                                   .side = newOrder.getOrderSide(), .isOwn = newOrder.isOwn()});
        ///responseEvent Resting - restul din newOrder(limit) a fost adaugat in OrderBook
    }

    rebalanceLevels();

    return responseEvents;
}

ResponseEvent OrderBook::processCancelOrder(OrderId id, Timestamp currentTime, bool isOwnRequest) {
    auto it = m_activeOrders.find(id);
    if (it == m_activeOrders.end())
        return { ResponseType::CancelFailed, id, m_symbol, currentTime, Side{}, isOwnRequest};

    OrderLocation& loc = it->second;
    Order& order = loc.it->value;
    Side side = order.getOrderSide();
    bool isOwn = order.isOwn();

    loc.level->totalQuantity -= order.getQuantity();
    loc.level->orders.erase(loc.it);

    if (loc.level->orders.empty()) {
        bool isRelevant = (loc.side == Side::Buy) ? m_relevantBids.contains(loc.price)
                                                   : m_relevantAsks.contains(loc.price);

        if (isRelevant)
            removeRelevantLevel(loc.side, loc.price);
        else
            removeOtherLevel(loc.side, loc.price);

        rebalanceLevels();
    }

    m_activeOrders.erase(it);
    return {ResponseType::Cancelled, id, m_symbol, currentTime, side, isOwn};
}

std::vector<ResponseEvent> OrderBook::processModifyOrder(OrderId id, Quantity newQty,
                                                            std::optional<Price> newPrice,
                                                            Cash availableCash, Timestamp currentTime,
                                                            bool isOwnRequest) {
    std::vector<ResponseEvent> responses;

    auto it = m_activeOrders.find(id);
    if (it == m_activeOrders.end()) {
        responses.push_back({ResponseType::ModifyFailed, id, m_symbol, currentTime, Side{}, isOwnRequest});
        return responses;
    }

    if (newQty <= 0) {
        responses.push_back({ResponseType::ModifyFailed, id, m_symbol, currentTime, it->second.side,
                             it->second.it->value.isOwn()});
        return responses;
    }

    Order original = it->second.it->value;

    processCancelOrder(id, currentTime, original.isOwn());

    Order modifiedOrder{id, original.getSymbol(), original.getTimeStamp(), original.getOrderSide(),
                      newQty, original.getOrderType(),
                      newPrice.has_value() ? newPrice : original.getLimitPrice(),
                      original.isOwn()};

    auto addResults = processAddOrder(modifiedOrder, availableCash, currentTime);

    responses.push_back({ResponseType::Modified, id, m_symbol, currentTime, modifiedOrder.getOrderSide(),
                         modifiedOrder.isOwn()});
    responses.insert(responses.end(), addResults.begin(), addResults.end());

    return responses;
}


std::string fmtPrice(Price p) {
    //Claude Opus 4.8 Medium effort
    std::ostringstream os;
    os << p / 100 << '.' << std::setfill('0') << std::setw(2) << p % 100;
    return os.str();
}

std::ostream& operator<<(std::ostream& out, const OrderBook& book) {
    std::string title = "ORDER BOOK FOR " + book.getSymbol();
    constexpr int totalWidth{ 60 };
    constexpr int colWidth{ 12 };

    out << std::string((totalWidth - static_cast<int>(title.size())) / 2, ' ')
        << title << "\n\n";

    out << std::left
        << std::setw(colWidth * 2) << "BIDS"
        << " | "
        << std::setw(colWidth * 2) << "ASKS" << '\n';

    out << std::setw(colWidth) << "Price"
        << std::setw(colWidth) << "Qty"
        << " | "
        << std::setw(colWidth) << "Price"
        << std::setw(colWidth) << "Qty" << '\n';

    out << std::string(totalWidth, '-') << '\n';

    auto bidIt{ book.getRelevantBids().begin() };
    auto askIt{ book.getRelevantAsks().begin() };

    const size_t maxLevels{ 20 };
    size_t printed{};

    while ((bidIt != book.getRelevantBids().end() || askIt != book.getRelevantAsks().end())
           && printed < maxLevels) {
        if (bidIt != book.getRelevantBids().end()) {
            out << std::setw(colWidth) << fmtPrice(bidIt->first)
                << std::setw(colWidth) << bidIt->second.totalQuantity;
            ++bidIt;
        }
        else
            out << std::setw(colWidth * 2) << "";

        out << " | ";

        if (askIt != book.getRelevantAsks().end()) {
            out << std::setw(colWidth) << fmtPrice(askIt->first)
                << std::setw(colWidth) << askIt->second.totalQuantity;
            ++askIt;
        }
        else
            out << std::setw(colWidth * 2) << "";

        out << '\n';
        ++printed;
           }

    if (printed == maxLevels)
        out << "... (truncated)\n";

    out << std::setfill(' ');
    return out;
}

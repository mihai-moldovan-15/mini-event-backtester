#include "OrderBook.hpp"

#include <format>
#include <stdexcept>

void OrderBook::validate() const {
    if (m_symbol.empty())
        throw std::invalid_argument("Missing book symbol");

    if (!m_asks.empty() &&!m_bids.empty() && m_asks.begin()->first < m_bids.begin()->first)
        throw std::logic_error("Book cannot be crossed");

    if (!m_bids.empty() && m_bids.rbegin()->first < 0)
        throw std::invalid_argument("Bid prices cannot be negative");

    for (const auto& [price, level] : m_bids) {
        if (level.totalQuantity <= 0)
            throw std::logic_error("Bid level has non-positive quantity at " + std::to_string(price));

        Quantity sum{};
        for (auto it = level.orders.getHead(); it != nullptr; it=it->next)
            sum += it->value.getQuantity();

        if (sum != level.totalQuantity)
            throw std::logic_error("Bid level totalQuantity mismatch at price " + std::to_string(price));
    }

    if (!m_asks.empty() && m_asks.begin()->first < 0)
        throw std::invalid_argument("Ask prices cannot be negative");

    for (const auto& [price, level] : m_asks) {
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

void OrderBook::recordTrade(const Order& incoming, const Order& existing, Price price, Quantity qty,
                             Timestamp currentTime, std::vector<ResponseEvent>& responses) {

    ResponseType incomingType{ incoming.getQuantity() - qty == 0 ? ResponseType::Filled : ResponseType::PartiallyFilled };
    ResponseType existingType {existing.getQuantity() - qty == 0 ? ResponseType::Filled : ResponseType::PartiallyFilled };

    responses.push_back({incomingType, incoming.getOrderId(), m_symbol,
                              currentTime, incoming.getOrderSide(),
                                incoming.isOwn(), qty, price});

    responses.push_back({ existingType, existing.getOrderId(), m_symbol,
                            currentTime, existing.getOrderSide(),
                               existing.isOwn(), qty ,price} );
}

std::vector<ResponseEvent> OrderBook::processAddOrder(const Order& order, Cash availableCash, Timestamp currentTime) {
    Order newOrder = order;
    std::vector<ResponseEvent> responseEvents{};

    if (newOrder.getOrderSide() == Side::Buy) {
        auto& levels = m_asks;
        auto it = levels.begin();
        Cash remainingCash = availableCash;
        bool cashExhausted = false;

        while (newOrder.getQuantity() > 0 && it != levels.end() && !cashExhausted) {
            Price existingPrice = it->first;
            if (newOrder.getOrderType() == OrderType::Limit && *newOrder.getLimitPrice() < existingPrice)
                break;                                                                                    ///aici era checkfill ul

            BookLevel& level = it->second;
            auto ordIt = level.orders.getHead();

            while (newOrder.getQuantity() > 0 && ordIt != nullptr && !cashExhausted) {
                Order& existing = ordIt->value;
                Quantity qty = std::min(newOrder.getQuantity(), existing.getQuantity());

                if (newOrder.isOwn()) {
                    Quantity affordableQty = static_cast<Quantity>(remainingCash / existingPrice);
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

            it = level.orders.empty() ? levels.erase(it) : std::next(it);
        }
    }
    else { ///Side::Sell
        auto& levels = m_bids;
        auto it = levels.begin();
        Cash remainingCash = availableCash;

        while (newOrder.getQuantity() > 0 && it != levels.end()) {
            Price existingPrice = it->first;
            if (newOrder.getOrderType() == OrderType::Limit && *newOrder.getLimitPrice() > existingPrice)
                break;

            BookLevel& level = it->second;
            auto ordIt = level.orders.getHead();

            while (newOrder.getQuantity() > 0 && ordIt != nullptr) {
                Order& existing = ordIt->value;
                Quantity qty = std::min(newOrder.getQuantity(), existing.getQuantity());

                if (existing.isOwn()) {
                    Quantity affordableQty = static_cast<Quantity>(remainingCash / existingPrice);
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

            it = level.orders.empty() ? levels.erase(it) : std::next(it);
        }
    }

    if (newOrder.getQuantity() > 0 && newOrder.getOrderType() == OrderType::Limit) {
        if (newOrder.getOrderSide() == Side::Buy) {
            auto [it, _] = m_bids.try_emplace(*newOrder.getLimitPrice(), &m_pool);
            BookLevel& level = it->second;
            level.orders.push_back(newOrder);
            level.totalQuantity += newOrder.getQuantity();
            auto newOrd = level.orders.getTail();
            m_activeOrders[newOrder.getOrderId()] = OrderLocation{Side::Buy, *newOrder.getLimitPrice(), &level, newOrd};
        }
        else {
            auto [it, _] = m_asks.try_emplace(*newOrder.getLimitPrice(), &m_pool);
            BookLevel& level = it->second;
            level.orders.push_back(newOrder);
            level.totalQuantity += newOrder.getQuantity();
            auto newOrd = level.orders.getTail();
            m_activeOrders[newOrder.getOrderId()] = OrderLocation{Side::Sell, *newOrder.getLimitPrice(), &level, newOrd};
        }

        responseEvents.push_back({ResponseType::Resting, newOrder.getOrderId(), m_symbol, currentTime,
                                   newOrder.getOrderSide(), newOrder.isOwn()});
        ///responseEvent Resting - restul din newOrder(limit) a fost adaugat in OrderBook
    }

    return responseEvents;
}

ResponseEvent OrderBook::processCancelOrder(OrderId id, Timestamp currentTime) {
    auto it = m_activeOrders.find(id);
    if (it == m_activeOrders.end())
        return { ResponseType::CancelFailed, id, m_symbol, currentTime, Side{}, true };

    OrderLocation& loc = it->second;
    Order& order = loc.it->value;
    Side side = order.getOrderSide();

    loc.level->totalQuantity -= order.getQuantity();
    loc.level->orders.erase(loc.it);

    if (loc.level->orders.empty())
        removeLevel(loc.side, loc.price);

    m_activeOrders.erase(it);
    return {ResponseType::Cancelled, id, m_symbol, currentTime, side, true};
}

std::vector<ResponseEvent> OrderBook::processModifyOrder(OrderId id, Quantity newQty,
                                                            std::optional<Price> newPrice,
                                                            Cash availableCash, Timestamp currentTime) {
    std::vector<ResponseEvent> responses;

    auto it = m_activeOrders.find(id);
    if (it == m_activeOrders.end()) {
        responses.push_back({ResponseType::ModifyFailed, id, m_symbol, currentTime, Side{}, true});
        return responses;
    }

    if (newQty <= 0) {
        responses.push_back({ResponseType::ModifyFailed, id, m_symbol, currentTime, it->second.side, true});
        return responses;
    }

    Order original = it->second.it->value;

    processCancelOrder(id, currentTime);

    Order modifiedOrder{id, original.getSymbol(), original.getTimeStamp(), original.getOrderSide(),
                      newQty, original.getOrderType(),
                      newPrice.has_value() ? newPrice : original.getLimitPrice(),
                      original.isOwn()};

    auto addResults = processAddOrder(modifiedOrder, availableCash, currentTime);

    responses.push_back({ResponseType::Modified, id, m_symbol, currentTime, modifiedOrder.getOrderSide(), true});
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

    auto bidIt{ book.getBids().begin() };
    auto askIt{ book.getAsks().begin() };

    const size_t maxLevels{ 20 };
    size_t printed{};

    while ((bidIt != book.getBids().end() || askIt != book.getAsks().end())
           && printed < maxLevels) {
        if (bidIt != book.getBids().end()) {
            out << std::setw(colWidth) << fmtPrice(bidIt->first)
                << std::setw(colWidth) << bidIt->second.totalQuantity;
            ++bidIt;
        }
        else
            out << std::setw(colWidth * 2) << "";

        out << " | ";

        if (askIt != book.getAsks().end()) {
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

#include "Event.hpp"
#include "Simulator.hpp"
#include "OrderBook.hpp"

void AddOrderEvent::execute(Simulator &simulator) {
    auto& book = simulator.getOrderBook(m_order.getSymbol());
    auto responses = book.processAddOrder(m_order, simulator.getPortfolio().getAvailableCash(), simulator.getCurrTimeStamp());

    for (auto& response : responses)
        simulator.scheduleEvent(std::make_unique<MarketReturnEvent>(
            simulator.getCurrTimeStamp() + simulator.getReturnLatency(), std::move(response)));
}

void CancelOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBookForOrder(m_orderId);
    auto response = book.processCancelOrder(m_orderId, simulator.getCurrTimeStamp());
    simulator.scheduleEvent(std::make_unique<MarketReturnEvent>(
            simulator.getCurrTimeStamp() + simulator.getReturnLatency(), std::move(response)));
}

void ModifyOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBookForOrder(m_orderId);
    auto responses = book.processModifyOrder(m_orderId, m_newQuantity, m_newLimitPrice,
                                              simulator.getPortfolio().getAvailableCash(), simulator.getCurrTimeStamp());
    for (auto& response : responses)
        simulator.scheduleEvent(std::make_unique<MarketReturnEvent>(
            simulator.getCurrTimeStamp() + simulator.getReturnLatency(), std::move(response)));
}

void MarketReturnEvent::execute(Simulator &simulator) {
    simulator.handleResponse(m_response);
}

void TimerEvent::execute(Simulator &simulator) {
    simulator.runStrategy();
    simulator.scheduleTimer();
}
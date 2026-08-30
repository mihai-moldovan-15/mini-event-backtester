#include "Event.hpp"
#include "Simulator.hpp"
#include "OrderBook.hpp"

void AddPersonalOrderEvent::execute(Simulator &simulator) {
    auto& book = simulator.getOrderBook(m_order.getSymbol());
    auto responses = book.processAddOrder(m_order, simulator.getPortfolio().getAvailableCash(), simulator.getCurrTimeStamp());

    for (auto& response : responses)
        simulator.scheduleEvent(std::make_unique<MarketReturnEvent>(
            simulator.getCurrTimeStamp() + simulator.getReturnLatency(), std::move(response)));
}

void CancelPersonalOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBookForOrder(m_orderId);
    auto response = book.processCancelOrder(m_orderId, simulator.getCurrTimeStamp(), true);
    simulator.scheduleEvent(std::make_unique<MarketReturnEvent>(
            simulator.getCurrTimeStamp() + simulator.getReturnLatency(), std::move(response)));
}

void CancelHistoricalOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBook(m_symbol);
    auto response = book.processCancelOrder(m_orderId, simulator.getCurrTimeStamp(), false);
    simulator.scheduleEvent(std::make_unique<MarketReturnEvent>(
        simulator.getCurrTimeStamp() + simulator.getReturnLatency(), std::move(response)));
}

void ModifyPersonalOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBookForOrder(m_orderId);
    auto responses = book.processModifyOrder(m_orderId, m_newQuantity, m_newLimitPrice,
                                              simulator.getPortfolio().getAvailableCash(), simulator.getCurrTimeStamp(), true);
    for (auto& response : responses)
        simulator.scheduleEvent(std::make_unique<MarketReturnEvent>(
            simulator.getCurrTimeStamp() + simulator.getReturnLatency(), std::move(response)));
}

void ModifyHistoricalOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBook(m_symbol);
    ///availableCash conteaza si aici: re-add ul poate lovi ordinele noastre rezidente
    auto responses = book.processModifyOrder(m_orderId, m_newQuantity, m_newLimitPrice,
                                              simulator.getPortfolio().getAvailableCash(), simulator.getCurrTimeStamp(), false);
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
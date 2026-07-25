#include "Event.hpp"
#include "Simulator.hpp"
#include "OrderBook.hpp"

void AddOrderEvent::execute(Simulator &simulator) {
    auto& book = simulator.getOrderBook(m_order.getSymbol());
    auto responses = book.processAddOrder(m_order, simulator.getCurrTimeStamp());

    for (const auto& response : responses)
        simulator.handleResponse(response);
}

void CancelOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBookForOrder(m_orderId);
    auto response = book.processCancelOrder(m_orderId, simulator.getCurrTimeStamp());
    simulator.handleResponse(response);
}

void ModifyOrderEvent::execute(Simulator& simulator) {
    auto& book = simulator.getOrderBookForOrder(m_orderId);
    auto responses = book.processModifyOrder(m_orderId, m_newQuantity, m_newLimitPrice, simulator.getCurrTimeStamp());
    for (const auto& response : responses)
        simulator.handleResponse(response);
}

void TimerEvent::execute(Simulator &simulator) {
    simulator.runStrategy();
    simulator.scheduleTimer();
}
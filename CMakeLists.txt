cmake_minimum_required(VERSION 3.20)
project(mini_event_backtester)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mini_event_backtester
        src/main.cpp
        src/Order.cpp
        src/Fill.cpp
        src/OrderBook.cpp
        src/Portfolio.cpp
        src/Simulator.cpp
        src/Event.cpp
        src/AStrategy.cpp
        "data structures/double_linked_list.hpp"
)

target_include_directories(mini_event_backtester PRIVATE include)
cmake_minimum_required(VERSION 3.16)
project(backtester CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(WITH_MYSQL "Build the MySQL data source (requires MySQL Connector/C++)" OFF)

add_library(backtester
  src/backtest.cpp
  src/data_handler.cpp
  src/events.cpp
  src/performance.cpp
  src/portfolio.cpp
  src/strategy.cpp
  src/time_utils.cpp
)
target_include_directories(backtester PUBLIC include)
target_compile_options(backtester PRIVATE -Wall -Wextra -Wpedantic)

if(WITH_MYSQL)
  find_library(MYSQLCPPCONN_LIB mysqlcppconn REQUIRED)
  target_sources(backtester PRIVATE src/mysql_config.cpp src/mysql_data_handler.cpp)
  target_link_libraries(backtester PUBLIC ${MYSQLCPPCONN_LIB})
  target_compile_definitions(backtester PUBLIC BACKTESTER_WITH_MYSQL)
endif()

add_executable(backtest apps/main.cpp)
target_link_libraries(backtest PRIVATE backtester)

enable_testing()
add_executable(backtester_tests tests/test_main.cpp)
target_link_libraries(backtester_tests PRIVATE backtester)
add_test(NAME unit_tests COMMAND backtester_tests)
add_test(NAME sample_backtest COMMAND backtest --csv-dir ${CMAKE_SOURCE_DIR}/data/sample
         --output ${CMAKE_BINARY_DIR}/equity_curve.csv)

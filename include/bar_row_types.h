#ifndef BAR_ROW_H
#define BAR_ROW_H


#include <chrono>
#include <string>

struct mysql_bar_row {
    std::chrono::system_clock::time_point datetime;
    double open;
    double high;
    double low;
    double close;
    double adj_close;
    double volume;
    // peut contenir des champs spécifiques à MySQL
};

#endif

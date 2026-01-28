#ifndef DASHBOARD_DATA_SOURCE_H
#define DASHBOARD_DATA_SOURCE_H

#include <string>
#include <mutex>
#include <cstdint>
#include "UiData.h"

namespace dashboard
{
class DashboardDataSource
{
public:
    DashboardDataSource(UiData& data);

    void updateData(std::uint32_t gear,
        std::uint32_t speed,
        bool status);

    [[nodiscard]]
    std::string getData() const;  // Returns JSON

private:
    mutable std::mutex m_mutex;
    UiData& m_data;
};
}

#endif
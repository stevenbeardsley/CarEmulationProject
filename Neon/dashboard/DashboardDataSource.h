#ifndef DASHBOARD_DATA_SOURCE_H
#define DASHBOARD_DATA_SOURCE_H

#include "UiData.h"

#include <cstdint>
#include <mutex>
#include <string>

namespace dashboard
{
class DashboardDataSource
{
public:
    DashboardDataSource();                 
    explicit DashboardDataSource(UiData initial);

    // Thread-safe setters
    void setGear(std::int32_t gear);
    void setSpeed(std::int32_t speed);
    void setRpm(std::int32_t rpm);
    void setMaxRpms(std::uint32_t maxRpms);
    void setEngineTemp(std::uint32_t temp);
    void setStatus(bool status);
    void setEngineFuel(std::uint32_t fuel);

    void addError(shared::can::headers::Error code, const std::string& msg);
    void clearErrors();


    UiData snapshot() const;

    std::string getDataJson() const;

private:
    mutable std::mutex m_mtx;
    UiData m_data;
};
}

#endif 
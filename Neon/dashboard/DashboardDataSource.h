#ifndef DASHBOARD_DATA_SOURCE_H
#define DASHBOARD_DATA_SOURCE_H

#include "UiData.h"

#include <cstdint>
#include <mutex>
#include <string>

namespace dashboard
{
    // Owns UiData and provides thread-safe updates + JSON export.
    class DashboardDataSource
    {
    public:
        DashboardDataSource();                 // default: zeroed + status=true
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

        // Convenience: update multiple fields atomically
        void update(std::int32_t speed, std::int32_t gear, std::int32_t rpm, bool status);

        // Thread-safe snapshot (consistent view)
        UiData snapshot() const;

        // JSON string for WebSocket payloads
        std::string getDataJson() const;

    private:
        mutable std::mutex m_mtx;
        UiData m_data;
    };
}

#endif 
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
        explicit DashboardDataSource(const UiData& initial);

        // Thread-safe setters
        void SetGear(std::int32_t gear);
        void SetSpeed(std::int32_t speed);
        void SetRpm(std::int32_t rpm);
        void SetMaxRpms(std::uint32_t maxRpms);
        void SetEngineTemp(std::uint32_t temp);
        void SetStatus(bool status);

        // Convenience: update multiple fields atomically
        void Update(std::int32_t speed, std::int32_t gear, std::int32_t rpm, bool status);

        // Thread-safe snapshot (consistent view)
        UiData Snapshot() const;

        // JSON string for WebSocket payloads
        std::string GetDataJson() const;

    private:
        mutable std::mutex m_mtx;
        UiData m_data;
    };
}

#endif 
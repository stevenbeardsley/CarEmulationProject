#include "DashboardDataSource.h"

#include <sstream>
#include "LogFile.h"

namespace dashboard
{
    DashboardDataSource::DashboardDataSource()
        : m_data{ 0, 0, 0, true } // assumes UiData has fields: m_gear, m_speed, m_rpm, m_status
    {
    }

    DashboardDataSource::DashboardDataSource(const UiData& initial)
        : m_data(initial)
    {
    }

    void DashboardDataSource::SetGear(std::int32_t gear)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_data.m_gear = gear;
    }

    void DashboardDataSource::SetSpeed(std::int32_t speed)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_data.m_speed = speed;
    }

    void DashboardDataSource::SetRpm(std::int32_t rpm)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_data.m_rpm = rpm;
    }

    void DashboardDataSource::SetStatus(bool status)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_data.m_status = status;
    }

    void DashboardDataSource::Update(std::int32_t speed, std::int32_t gear, std::int32_t rpm, bool status)
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_data.m_speed = speed;
        m_data.m_gear = gear;
        m_data.m_rpm = rpm;
        m_data.m_status = status;
    }

    UiData DashboardDataSource::Snapshot() const
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_data; // copy
    }

    std::string DashboardDataSource::GetDataJson() const
    {
        const UiData d = Snapshot();

        // Simple JSON (no external deps). Adjust keys to match your C# parser.
        std::ostringstream oss;
        oss << "{"
            << "\"speed\":" << d.m_speed << ","
            << "\"gear\":" << d.m_gear << ","
            << "\"rpms\":" << d.m_rpm << ","
            << "\"status\":" << (d.m_status ? "true" : "false")
            << "}";

        LogFile::Info(oss.str());
        return oss.str();
    }
}

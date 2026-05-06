#include "DashboardDataSource.h"

#include <sstream>
#include "LogFile.h"

namespace dashboard
{
    DashboardDataSource::DashboardDataSource()
        : m_data{ 0, 0, 0, 0, true } 
    {
    }

    DashboardDataSource::DashboardDataSource(UiData initial)
        : m_data(std::move(initial))
    {
    }

    void DashboardDataSource::setGear(const std::int32_t gear)
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_gear = gear;
    }

    void DashboardDataSource::setSpeed(std::int32_t speed)
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_speed = speed;
    }

    void DashboardDataSource::setEngineFuel(std::uint32_t fuel)
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_fuel = fuel;
    }

    void DashboardDataSource::setRpm(std::int32_t rpm)
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_rpm = rpm;
    }

    void DashboardDataSource::setMaxRpms(std::uint32_t maxRpm)
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_maxRpms = maxRpm;
    }

    void DashboardDataSource::setEngineTemp(std::uint32_t temp)
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_engineTemp = temp;
    }

    void DashboardDataSource::setStatus(bool status)
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_status = status;
    }

    void DashboardDataSource::addError(const shared::can::headers::Error code,
        const std::string& msg) 
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_activeErrors.push_back({ code, msg });
    }

    void DashboardDataSource::clearErrors() 
    {
	    std::scoped_lock lk(m_mtx);
        m_data.m_activeErrors.clear();
    }

    UiData DashboardDataSource::snapshot() const
    {
	    std::scoped_lock lk(m_mtx);
        return m_data; 
    }

    std::string DashboardDataSource::getDataJson() const
    {
        const auto d = snapshot();

        std::ostringstream oss;
        oss << "{"
            << "\"speed\":" << d.m_speed << ","
            << "\"gear\":" << d.m_gear << ","
            << "\"rpms\":" << d.m_rpm << ","
            << "\"maxRpms\":" << d.m_maxRpms<< ","
            << "\"engineTemp\":" << d.m_engineTemp << ","
            << "\"fuel\":" << d.m_fuel << ","
            << "\"errors\": [";
        for (size_t i = 0; i < d.m_activeErrors.size(); ++i) {
            oss << "{" 
                << "\"code\":" << static_cast<std::int32_t>(d.m_activeErrors[i].m_code) << ","
                << "\"msg\":\"" << d.m_activeErrors[i].m_message << "\""
                << "}"; 

            if (i < d.m_activeErrors.size() - 1)
            {
                oss << ",";
            }
        }

            oss << "],"
            << "\"status\":" << (d.m_status ? "true" : "false")
            << "}";

        LogFile::info(oss.str());
        return oss.str();
    }
}

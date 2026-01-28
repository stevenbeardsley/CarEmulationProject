#include "DashboardDataSource.h"

namespace dashboard
{

DashboardDataSource::DashboardDataSource(UiData& data) :
    m_data(data)
{
}

std::string DashboardDataSource::getData() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string jsonStr;
    jsonStr += "{\n";
    jsonStr += "\"speed\": \"" + std::to_string(m_data.m_speed) + "\", ";
    jsonStr += "\"gear\": \"" + std::to_string(m_data.m_gear) + "\", ";
    jsonStr += "\"status\": \"" + std::string(m_data.m_status ? "true" : "false") + "\"";
    jsonStr += "\n}";
    return jsonStr; // TODO: Update the datasource to account for the new data
}

void DashboardDataSource::updateData(
    std::uint32_t gear,
    std::uint32_t speed,
    bool status)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.m_gear = gear;
    m_data.m_speed = speed;
    m_data.m_status = status;
}


}

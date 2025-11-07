#include "DashboardDataSource.h"

namespace dashboard
{
void DashboardDataSource::updateData(int speed, bool status)
{
    std::lock_guard<std::mutex> lock(mutex_);
    m_speed = speed;
    m_status = status;
}

std::string DashboardDataSource::getData() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string jsonStr;
    jsonStr += "{ \n";
    jsonStr += "\"    speed\": " + std::to_string(m_speed) + ", ";
    jsonStr += "\"    status\": " + std::string(m_status ? "true" : "false");
    jsonStr += " \n}";
    return jsonStr;
}
}
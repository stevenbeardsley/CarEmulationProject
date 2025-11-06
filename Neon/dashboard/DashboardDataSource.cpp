#include "DashboardDataSource.h"

namespace dashboard
{

void DashboardDataSource::updateData(const std::string& newData)
{
    std::lock_guard<std::mutex> lock(mutex_);
    m_data = newData;
}

std::string DashboardDataSource::getData() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return m_data;
}

std::string DashboardDataSource::toJson(int speed, bool status)
{
    std::string jsonStr;
    jsonStr += "{ ";
    jsonStr += "\"speed\": " + std::to_string(speed) + ", ";
    jsonStr += "\"status\": " + std::string(status ? "true" : "false");
    jsonStr += " }";
    return jsonStr;
}

}

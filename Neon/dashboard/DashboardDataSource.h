#ifndef DASHBOARD_DATA_SOURCE_H
#define DASHBOARD_DATA_SOURCE_H

#include <string>
#include <mutex>

namespace dashboard
{
class DashboardDataSource
{
public:
    DashboardDataSource() : m_speed(0), m_status(false) {}

    void updateData(int speed, bool status);
    std::string getData() const;  // Returns JSON

private:
    mutable std::mutex mutex_;
    int m_speed;
    bool m_status;
};
}

#endif
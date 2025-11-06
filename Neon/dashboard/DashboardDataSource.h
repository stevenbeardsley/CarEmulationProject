#include <string>
#include <mutex>
namespace dashboard
{

class DashboardDataSource
{
public:
    void updateData(const std::string& newData);
    std::string getData() const;
    std::string toJson(int speed, bool status);

private:
    mutable std::mutex mutex_;
    std::string m_data;
};

}
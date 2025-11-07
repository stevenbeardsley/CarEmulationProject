using System;

namespace SimulationPlatform.Models;
internal class DashboardModel
{
    public string m_speed { get;set; } = string.Empty;
    public string m_status { get; set; } = string.Empty;
    public DateTime m_timestamp { get; set; } = DateTime.UtcNow;

}

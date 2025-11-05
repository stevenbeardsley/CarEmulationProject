using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimulationPlatform.Models;
internal class DashboardModel
{
    public string m_status { get;set; } = string.Empty;
    public int m_pid { get; set; }
    public DateTime m_timestamp { get; set; } = DateTime.UtcNow;

}

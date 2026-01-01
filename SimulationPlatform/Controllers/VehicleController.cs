using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SimulationPlatform.Controllers;
public class VehicleController
{
    private readonly ICommandSender m_httpClient;

    public VehicleController(HttpClient commandSender)
    {
        m_httpClient = commandSender;
    }

    public async Task ShiftUpAsync()
    {
        await m_httpClient.SendAsync("command", "gear_delta", +1);
    }

    public async Task SetThrottleAsync(double value)
    {
        await m_httpClient.SendAsync("command", "throttle", value);
    }
}

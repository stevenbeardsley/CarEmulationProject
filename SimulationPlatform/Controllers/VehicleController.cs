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
        await m_httpClient.SendAsync("carCommands", "gear_delta", 2);
    }

    public async Task SetThrottleAsync(double value)
    {
        await m_httpClient.SendAsync("carCommands", "throttle", value);
    }
}

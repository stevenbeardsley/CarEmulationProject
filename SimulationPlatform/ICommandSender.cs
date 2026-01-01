using System.Threading.Tasks;

namespace SimulationPlatform;
public interface ICommandSender
{
    Task<bool> SendAsync(string endpoint, string field, object value);
}


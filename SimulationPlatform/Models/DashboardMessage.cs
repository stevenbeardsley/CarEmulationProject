using System.Text.Json;

namespace SimulationPlatform.Models
{
public class DashboardMessage
{
    public string m_speed { get; set; } = "-";
    public string m_status { get; set; } = "-";

    public static DashboardMessage? FromJson(string json)
    {
        try
        {
            var doc = JsonDocument.Parse(json);
            var root = doc.RootElement;

            return new DashboardMessage
            {
                m_speed = root.TryGetProperty("speed", out var s) ? s.GetString() ?? "-" : "-",
                m_status = root.TryGetProperty("status", out var _) ? s.GetString() ?? "-" : "-",
            };
        }
        catch
        {
            return null;
        }
    }

    public override string ToString() => $"Speed: {m_speed}, Status: {m_status}";
}
}

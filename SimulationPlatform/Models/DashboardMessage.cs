using System.Text.Json;

namespace SimulationPlatform.Models
{
    public class DashboardMessage
    {
        public string Status { get; set; } = "-";
        public int Pid { get; set; } = 0;

        public static DashboardMessage? FromJson(string json)
        {
            try
            {
                var doc = JsonDocument.Parse(json);
                var root = doc.RootElement;

                return new DashboardMessage
                {
                    Status = root.TryGetProperty("status", out var s) ? s.GetString() ?? "-" : "-",
                    Pid = root.TryGetProperty("pid", out var p) ? p.GetInt32() : 0
                };
            }
            catch
            {
                return null;
            }
        }

        public override string ToString() => $"Status: {Status}, PID: {Pid}";
    }
}

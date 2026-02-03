using System.Text.Json;
using System.Text.Json.Serialization;

namespace SimulationPlatform.Models
{
    public class DashboardMessage
    {
        [JsonPropertyName("speed")]
        public string? Speed
        {
            get; set;
        }

        [JsonPropertyName("gear")]
        public string? Gear
        {
            get; set;
        }

        [JsonPropertyName("rpm")]
        public string? Rpms
        {
            get; set;
        }

        [JsonPropertyName("status")]
        public string? Status
        {
            get; set;
        }

        public static DashboardMessage? FromJson(string json)
        {
            try
            {
                return JsonSerializer.Deserialize<DashboardMessage>(json);
            }
            catch
            {
                return null;
            }
        }

        public CarData ToCarData()
        {
            var speed = int.TryParse(Speed, out var parsedSpeed) ? parsedSpeed : 0;
            var gear = int.TryParse(Gear, out var parsedGear) ? parsedGear : 0;
            var rpms = int.TryParse(Rpms, out var parsedRpms) ? parsedRpms : 0;
            var status = bool.TryParse(Status, out var b) && b;

            return new CarData(speed, gear, rpms, status);
        }

    }
}

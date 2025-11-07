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
            // TODO: Impplement 
            return new CarData(Speed ?? "-", Status ?? "-");
        }
    }
}

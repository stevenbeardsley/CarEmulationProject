using System.Text.Json;
using System.Text.Json.Serialization;

namespace SimulationPlatform.Models
{
    public class DashboardMessage
    {
        [JsonPropertyName("speed")]
        public int Speed
        {
            get; set;
        }

        [JsonPropertyName("gear")]
        public int Gear
        {
            get; set;
        }

        [JsonPropertyName("rpms")]
        public int Rpms
        {
            get; set;
        }

        [JsonPropertyName("maxRpms")]
        public int MaxRpms
        {
            get; set;
        }


        [JsonPropertyName("status")]
        public bool Status
        {
            get; set;
        }

        public static DashboardMessage? FromJson(string json)
        {
            try
            {
                return JsonSerializer.Deserialize<DashboardMessage>(json);
            }
            catch (JsonException)
            {
                return null;
            }
        }

        public CarData ToCarData()
        {
            return new CarData(Speed, Gear, Rpms, MaxRpms, Status);
        }

    }
}

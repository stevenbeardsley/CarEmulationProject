using System.Text.Json;
using System.Text.Json.Serialization;
using System.Collections.Generic;

namespace SimulationPlatform.Models
{
    public class ErrorMessageJson
    {
        [JsonPropertyName("code")]
        public int Code
        {
            get; set;
        }

        [JsonPropertyName("msg")]
        public string Text { get; set; } = string.Empty;
    }

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

        [JsonPropertyName("engineTemp")]
        public int EngineTemp
        {
            get; set;
        }

        [JsonPropertyName("fuel")]
        public int Fuel
        {
            get; set;
        }

        [JsonPropertyName("status")]
        public bool Status
        {
            get; set;
        }

        [JsonPropertyName("errors")]
        public List<ErrorMessageJson> Errors { get; set; } = new List<ErrorMessageJson>();

        public static DashboardMessage? FromJson(string json)
        {
            try
            {
                var options = new JsonSerializerOptions { AllowTrailingCommas = true };
                return JsonSerializer.Deserialize<DashboardMessage>(json, options);
            }
            catch (JsonException)
            {
                return null;
            }
        }

        public CarData ToCarData()
        {
            //var errors = new List<ErrorMessage>();
            //foreach (var msg in Errors)
            //{
            //    errors.Add(new ErrorMessage(msg.Code, msg.Text));
            //}
            return new CarData(Speed, Gear, Rpms, MaxRpms, EngineTemp, Fuel, Status, Errors);
        }
    }
}
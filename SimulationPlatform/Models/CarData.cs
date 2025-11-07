using System;

namespace SimulationPlatform.Models
{
    public class CarData
    {
        public string Speed
        {
            get;
        }
        public string Status
        {
            get;
        }

        public CarData(string speed, string status)
        {
            Speed = speed;
            Status = status;
        }

        public CarData()
        {
            Speed = "Unknown";
            Status = "Unknown";
        }
    }
}

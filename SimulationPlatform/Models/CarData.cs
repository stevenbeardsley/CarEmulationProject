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

        public string Gear
        {
            get;
        }

        public CarData(string speed, string gear, string status)
        {
            Speed = speed;
            Status = status;
            Gear = gear;
        }

        public CarData()
        {
            Speed = "Unknown";
            Status = "Unknown";
            Gear = "Unknown";
        }
    }
}

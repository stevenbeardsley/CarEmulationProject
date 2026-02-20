using Microsoft.UI;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

namespace SimulationPlatform.Models
{
    public class CarData
    {
        private readonly object _lock = new();

        private int _speed;
        private bool _status;
        private int _gear;
        private int _rpms;
        private int _maxRpms;
        private int _engineTemp;
        private int _fuel;

        public CarData(int speed, int gear, int rpms, int maxRpms, int engineTemp, int fuel, bool status)
        {
            _speed = speed;
            _gear = gear;
            _rpms = rpms;
            _maxRpms = maxRpms;
            _engineTemp = engineTemp;
            _fuel = fuel;
            _status = status;
        }

        public CarData()
        {
            _speed = 0;
            _gear = 0;
            _rpms = 0;
            _maxRpms = 0;
            _engineTemp = 0;
            _fuel = 0;
            _status = true;
        }

        public int Speed
        {
            get
            {
                lock (_lock)
                {
                    return _speed;
                }
            }
            set
            {
                lock (_lock)
                {
                    _speed = value;
                }
            }
        }

        public int Gear
        {
            get
            {
                lock (_lock)
                {
                    return _gear;
                }
            }
            set
            {
                lock (_lock)
                {
                    _gear = value;
                }
            }
        }

        public int Rpms
        {
            get
            {
                lock (_lock)
                {
                    return _rpms;
                }
            }
            set
            {
                lock (_lock)
                {
                    _rpms = value;
                }
            }
        }

        public int MaxRpms
        {
            get
            {
                lock (_lock)
                {
                    return _maxRpms;
                }
            }
            set
            {
                lock (_lock)
                {
                    _maxRpms = value;
                }
            }
        }

        public int EngineTemp
        {
            get
            {
                lock (_lock)
                {
                    return _engineTemp;
                }
            }
            set
            {
                lock (_lock)
                {
                    _engineTemp = value;
                }
            }
        }

        public int Fuel
        {
            get
            {
                lock (_lock)
                {
                    return _fuel;
                }
            }
            set
            {
                lock (_lock)
                {
                    _fuel = value;
                }
            }
        }


        public bool Status
        {
            get
            {
                lock (_lock)
                {
                    return _status;
                }
            }
            set
            {
                lock (_lock)
                {
                    _status = value;
                }
            }
        }
    }
}

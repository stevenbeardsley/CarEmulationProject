namespace SimulationPlatform.Models
{
    public class CarData
    {
        private readonly object _lock = new();

        private int _speed;
        private bool _status;
        private int _gear;
        private int _rpms;

        public CarData(int speed, int gear, int rpms, bool status)
        {
            _speed = speed;
            _gear = gear;
            _rpms = rpms;
            _status = status;
        }

        public CarData()
        {
            _speed = 0;
            _gear = 0;
            _rpms = 0;
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

        /// <summary>
        /// Atomically update all fields together
        /// (important to avoid mixed-state reads)
        /// </summary>
        public void Update(int speed, int gear, int rpms, bool status)
        {
            lock (_lock)
            {
                _speed = speed;
                _gear = gear;
                _rpms = rpms;
                _status = status;
            }
        }

        /// <summary>
        /// Atomically read a consistent snapshot
        /// </summary>
        public (int Speed, int Gear, int Rpms, bool Status) Snapshot()
        {
            lock (_lock)
            {
                return (_speed, _gear, _rpms, _status);
            }
        }
    }
}

namespace SimulationPlatform.Models
{
    public class CarData
    {
        private readonly object _lock = new();

        private int _speed;
        private bool _status;
        private int _gear;

        public CarData(int speed, int gear, bool status)
        {
            _speed = speed;
            _gear = gear;
            _status = status;
        }

        public CarData()
        {
            _speed = 0;
            _gear = 0;
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
        public void Update(int speed, int gear, bool status)
        {
            lock (_lock)
            {
                _speed = speed;
                _gear = gear;
                _status = status;
            }
        }

        /// <summary>
        /// Atomically read a consistent snapshot
        /// </summary>
        public (int Speed, int Gear, bool Status) Snapshot()
        {
            lock (_lock)
            {
                return (_speed, _gear, _status);
            }
        }
    }
}

using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Nodes;

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
        private Dictionary<int, string> _errors = new();

        public CarData(int speed, int gear, int rpms, int maxRpms, int engineTemp, int fuel, bool status, List<ErrorMessageJson> errors)
        {
            _speed = speed;
            _gear = gear;
            _rpms = rpms;
            _maxRpms = maxRpms;
            _engineTemp = engineTemp;
            _fuel = fuel;
            _status = status;
            UpdateErrors(errors);
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
            _errors = new Dictionary<int, string>();
        }

        public void UpdateErrors(List<ErrorMessageJson>? incomingErrors)
        {
            if (incomingErrors == null) return;

            lock (_lock)
            {
                // 1. Get incoming codes as a HashSet for O(1) lookup
                var incomingCodes = incomingErrors.Select(e => e.Code).ToHashSet();

                // 2. Remove errors that are no longer present in the JSON
                var codesToRemove = _errors.Keys.Where(code => !incomingCodes.Contains(code)).ToList();
                foreach (var code in codesToRemove)
                {
                    _errors.Remove(code);
                }

                // 3. Add only the new errors that don't exist in our dictionary yet
                foreach (var incoming in incomingErrors)
                {
                    if (!_errors.ContainsKey(incoming.Code))
                    {
                        _errors.Add(incoming.Code, incoming.Text);
                    }
                    // Optional: Update text if the message changed but code remained same
                    else if (_errors[incoming.Code] != incoming.Text)
                    {
                        _errors[incoming.Code] = incoming.Text;
                    }
                }
            }
        }


        public int Speed
        {
            get
            {
                lock (_lock) return _speed;
            }
            set
            {
                lock (_lock) _speed = value;
            }
        }

        public int Gear
        {
            get
            {
                lock (_lock) return _gear;
            }
            set
            {
                lock (_lock) _gear = value;
            }
        }

        public int Rpms
        {
            get
            {
                lock (_lock) return _rpms;
            }
            set
            {
                lock (_lock) _rpms = value;
            }
        }

        public int MaxRpms
        {
            get
            {
                lock (_lock) return _maxRpms;
            }
            set
            {
                lock (_lock) _maxRpms = value;
            }
        }

        public int EngineTemp
        {
            get
            {
                lock (_lock) return _engineTemp;
            }
            set
            {
                lock (_lock) _engineTemp = value;
            }
        }

        public int Fuel
        {
            get
            {
                lock (_lock) return _fuel;
            }
            set
            {
                lock (_lock) _fuel = value;
            }
        }

        public bool Status
        {
            get
            {
                lock (_lock) return _status;
            }
            set
            {
                lock (_lock) _status = value;
            }
        }

        public Dictionary<int, string> Errors
        {
            get
            {
                lock (_lock) return _errors;
            }
        }

        public bool HasErrors
        {
            get
            {
                lock (_lock) return _errors.Count > 0;
            }
        }
    }
}
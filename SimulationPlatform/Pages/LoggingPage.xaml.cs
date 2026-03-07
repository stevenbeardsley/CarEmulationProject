using System.ComponentModel;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Models;
using Microsoft.UI.Dispatching;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace SimulationPlatform.Pages
{
    public class ErrorViewModel : INotifyPropertyChanged
    {
        private string _code;
        private string _text = string.Empty;

        public event PropertyChangedEventHandler? PropertyChanged;

        public string Code
        {
            get => _code;
            set
            {
                if (_code != value)
                {
                    _code = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Code)));
                }
            }
        }

        public string Message
        {
            get => _text;
            set
            {
                if (_text != value)
                {
                    _text = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Message)));
                }
            }
        }

        public ErrorViewModel(ErrorMessage model)
        {
            _code = model.Code.ToString();
            _text = model.Message;
        }
    }

    public sealed partial class LoggingPage : Page, INotifyPropertyChanged
    {
        private string m_speed = string.Empty;
        private string m_gear = string.Empty;
        private string m_rpm = string.Empty;
        private string m_maxRpms = string.Empty;
        private string m_engineTemp = string.Empty;
        private string m_fuel = string.Empty;
        private string m_status = string.Empty;

        public ObservableCollection<ErrorViewModel> ActiveErrors { get; } = new();
        public event PropertyChangedEventHandler? PropertyChanged;

        public LoggingPage()
        {
            this.InitializeComponent();
            DataContext = this;
        }

        public string Speed
        {
            get => m_speed;
            set
            {
                if (m_speed != value)
                {
                    m_speed = value;
                    OnPropertyChanged(nameof(Speed));
                }
            }
        }

        public string Gear
        {
            get => m_gear;
            set
            {
                if (m_gear != value)
                {
                    m_gear = value;
                    OnPropertyChanged(nameof(Gear));
                }
            }
        }

        public string Rpm
        {
            get => m_rpm;
            set
            {
                if (m_rpm != value)
                {
                    m_rpm = value;
                    OnPropertyChanged(nameof(Rpm));
                }
            }
        }

        public string MaxRpms
        {
            get => m_maxRpms;
            set
            {
                if (m_maxRpms != value)
                {
                    m_maxRpms = value;
                    OnPropertyChanged(nameof(MaxRpms));
                }
            }
        }

        public string EngineTemp
        {
            get => m_engineTemp;
            set
            {
                if (m_engineTemp != value)
                {
                    m_engineTemp = value;
                    OnPropertyChanged(nameof(EngineTemp));
                }
            }
        }

        public string Fuel
        {
            get => m_fuel;
            set
            {
                if (m_fuel != value)
                {
                    m_fuel = value;
                    OnPropertyChanged(nameof(Fuel));
                }
            }
        }

        public string Status
        {
            get => m_status;
            set
            {
                if (m_status != value)
                {
                    m_status = value;
                    OnPropertyChanged(nameof(Status));
                }
            }
        }

        private void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private void UpdateCarData(CarData carData)
        {
            DispatcherQueue.TryEnqueue(() =>
            {
                // 1. Update standard telemetry
                Speed = carData.Speed.ToString();
                Status = carData.Status.ToString();
                Rpm = carData.Rpms.ToString();
                MaxRpms = carData.MaxRpms.ToString();
                EngineTemp = carData.EngineTemp.ToString();
                Fuel = carData.Fuel.ToString();
                Gear = carData.Gear.ToString();

                // 2. Get snapshot of current errors (Dictionary<int, string>)
                var latestErrors = carData.Errors;

                // 3. REMOVE: Remove errors that are no longer present
                for (var i = ActiveErrors.Count - 1; i >= 0; i--)
                {
                    // Parse the code back to int to compare with dictionary keys
                    if (int.TryParse(ActiveErrors[i].Code, out var existingCode))
                    {
                        if (!latestErrors.ContainsKey(existingCode))
                        {
                            ActiveErrors.RemoveAt(i);
                        }
                    }
                }

                // 4. ADD/UPDATE: Add new or update existing
                foreach (var kvp in latestErrors)
                {
                    var stringCode = kvp.Key.ToString();
                    var existing = ActiveErrors.FirstOrDefault(e => e.Code == stringCode);

                    if (existing == null)
                    {
                        if (existing == null)
                        {
                            // 1. Create the Model (The data container)
                            var model = new ErrorMessage(kvp.Key, kvp.Value);

                            // 2. Create the ViewModel (The UI wrapper)
                            var vm = new ErrorViewModel(model);

                            // 3. Add to the collection
                            ActiveErrors.Add(vm);
                        }
                    }
                }
            });
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            UpdateCarData(App.m_model.m_carData);
            App.m_model.m_webSocketController.CarDataReceived += UpdateCarData;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            App.m_model.m_webSocketController.CarDataReceived -= UpdateCarData;
        }
    }
}
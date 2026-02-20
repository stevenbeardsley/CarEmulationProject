using System.ComponentModel;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Models;
using Microsoft.UI.Dispatching;

namespace SimulationPlatform.Pages
{
    public sealed partial class LoggingPage : Page, INotifyPropertyChanged
    {
        private string m_speed = string.Empty;
        private string m_gear = string.Empty;
        private string m_rpm = string.Empty;
        private string m_maxRpms = string.Empty;
        private string m_engineTemp = string.Empty;
        private string m_fuel = string.Empty;
        private string m_status = string.Empty;
        
        public event PropertyChangedEventHandler? PropertyChanged;

        public LoggingPage()
        {
            this.InitializeComponent();
            DataContext = App.m_model;
            // TODO: Bind the updates, so this page updates dynamically 
        }

        // Property for Speed
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
                Speed = carData.Speed.ToString();
                Status = carData.Status.ToString();
                Rpm = carData.Rpms.ToString();
                MaxRpms = carData.MaxRpms.ToString();
                EngineTemp = carData.EngineTemp.ToString(); // TODO: Are the property raisers needed?
                Fuel = carData.Fuel.ToString();
                Gear = carData.Gear.ToString();
                OnPropertyChanged(nameof(Speed));
                OnPropertyChanged(nameof(Status));
                OnPropertyChanged(nameof(Gear));
                OnPropertyChanged(nameof(Rpm));
                OnPropertyChanged(nameof(Fuel));
                OnPropertyChanged(nameof(MaxRpms));
                OnPropertyChanged(nameof(EngineTemp));
            });
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // Example: Pull data from model
            Speed = App.m_model.m_carData.Speed.ToString();
            Status = App.m_model.m_carData.Status.ToString();
            Gear = App.m_model.m_carData.Gear.ToString();
            Rpm = App.m_model.m_carData.Rpms.ToString();
            MaxRpms = App.m_model.m_carData.MaxRpms.ToString();
            EngineTemp = App.m_model.m_carData.EngineTemp.ToString();
            Fuel = App.m_model.m_carData.Fuel.ToString();
            App.m_model.m_webSocketController.CarDataReceived += UpdateCarData;
        }


        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unsubscribe from model changes (if you add them later)
            App.m_model.m_webSocketController.CarDataReceived -= UpdateCarData;
        }
    }
}

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
                Gear = carData.Gear.ToString();
                OnPropertyChanged(nameof(Speed));
                OnPropertyChanged(nameof(Status));
                OnPropertyChanged(nameof(Gear));
                OnPropertyChanged(nameof(Rpm));
            });
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // Example: Pull data from model
            Speed = App.m_model.m_carData.Speed.ToString();
            Status = App.m_model.m_carData.Status.ToString();
            Gear = App.m_model.m_carData.Gear.ToString();
            Rpm = App.m_model.m_carData.Rpms.ToString();
            App.m_model.m_webSocketController.CarDataReceived += UpdateCarData;
        }


        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unsubscribe from model changes (if you add them later)
            App.m_model.m_webSocketController.CarDataReceived -= UpdateCarData;
        }
    }
}

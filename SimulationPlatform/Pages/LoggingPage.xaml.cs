using System;
using System.ComponentModel;
using System.Transactions;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Controllers;
using SimulationPlatform.Models;

namespace SimulationPlatform.Pages
{
    public sealed partial class LoggingPage : Page, INotifyPropertyChanged
    {
        private string m_speed = string.Empty;
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

        // Property for Status
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
            Speed = carData.Speed;
            Status = carData.Status;
            OnPropertyChanged(nameof(Speed));
            OnPropertyChanged(nameof(Status));

        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // Example: Pull data from model
            Speed = App.m_model.m_carData.Speed;
            Status = App.m_model.m_carData.Status;
            App.m_model.m_webSocketController.CarDataReceived += UpdateCarData;
        }


        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unsubscribe from model changes (if you add them later)
            App.m_model.m_webSocketController.CarDataReceived -= UpdateCarData;
        }
    }
}

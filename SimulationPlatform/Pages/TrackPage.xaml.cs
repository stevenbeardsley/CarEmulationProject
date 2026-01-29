using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Models;

namespace SimulationPlatform.Pages
{
    public sealed partial class TrackPage : Page, INotifyPropertyChanged
    {

        private readonly AppModel m_model;
        public event PropertyChangedEventHandler? PropertyChanged;
        private string m_speed = string.Empty;
        private string m_gear = string.Empty;

        public bool m_downShiftEnabled
        {
            get
            {
                var enabled = true;
                if (m_gear == "0")
                {
                    enabled = false;
                }
                return enabled;
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


        public TrackPage()
        {
            InitializeComponent();
            m_model = App.m_model; 
        }
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            Speed = App.m_model.m_carData.Speed.ToString();
            Gear = App.m_model.m_carData.Gear.ToString();
            App.m_model.m_webSocketController.CarDataReceived += UpdateCarData;
        }

        private void OnPropertyChanged(string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unsubscribe from model changes (if you add them later)
            App.m_model.m_webSocketController.CarDataReceived -= UpdateCarData;
        }

        private void UpdateCarData(CarData carData)
        {
            DispatcherQueue.TryEnqueue(() =>
            {
                Speed = carData.Speed.ToString();
                Gear = carData.Gear.ToString();
                OnPropertyChanged(nameof(Speed));
                OnPropertyChanged(nameof(Gear));
                OnPropertyChanged(nameof(m_downShiftEnabled));
            });
        }

        private async void GearShiftDown_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await m_model.VehicleController.ShiftDownAsync();
            }
            catch (Exception ex)
            {
                // TODO: log or surface error
                System.Diagnostics.Debug.WriteLine(ex.Message);
                OnPropertyChanged(nameof(m_downShiftEnabled));
            }
        }

        private async void GearShiftUp_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await m_model.VehicleController.ShiftUpAsync();
            }
            catch (Exception ex)
            {
                // TODO: log or surface error
                System.Diagnostics.Debug.WriteLine(ex.Message);
            }
        }

    }
}
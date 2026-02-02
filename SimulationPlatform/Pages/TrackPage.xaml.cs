using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
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
        private double _acceleration = 25; // default matches your old Value="25"
        private bool _isDraggingAcceleration;

        public double Acceleration
        {
            get => _acceleration;
            set
            {
                if (Math.Abs(_acceleration - value) > 0.001)
                {
                    _acceleration = value;
                    OnPropertyChanged();
                    OnPropertyChanged(nameof(AccelerationText));
                }
            }
        }
        public string AccelerationText => $"{Acceleration:0}%";


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
            // Slider eats pointer events, so listen even if already handled
            AccelerationSlider.AddHandler(
                UIElement.PointerReleasedEvent,
                new Microsoft.UI.Xaml.Input.PointerEventHandler(AccelerationSlider_PointerReleased),
                handledEventsToo: true);

            AccelerationSlider.AddHandler(
                UIElement.PointerCaptureLostEvent,
                new Microsoft.UI.Xaml.Input.PointerEventHandler(AccelerationSlider_PointerCaptureLost),
                handledEventsToo: true);
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

        private async void accelerationSelected()
        {
            try
            {
                await m_model.VehicleController.SetThrottleAsync(Acceleration);
            }
            catch (Exception ex)
            {
                // TODO: log or surface error
                System.Diagnostics.Debug.WriteLine(ex.Message);
            }
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

        private void AccelerationSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            // With x:Bind TwoWay, you technically don't need this at all.
            // But if you keep it, don't do extra commit logic here.
            if (Math.Abs(Acceleration - e.NewValue) > 0.001)
                Acceleration = e.NewValue;
        }

        private void AccelerationSlider_PointerReleased(object sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            // Commit/send only when the user lets go
            accelerationSelected();
        }

        private void AccelerationSlider_PointerCaptureLost(object sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            // Also commit if capture is lost (pointer leaves window etc.)
            accelerationSelected();
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
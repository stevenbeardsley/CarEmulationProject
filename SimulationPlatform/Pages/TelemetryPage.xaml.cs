using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Models;
using Windows.UI;

namespace SimulationPlatform.Pages
{
    public sealed partial class TelemetryPage : Page
    {
        private static readonly Color ColourSpeed = Color.FromArgb(255, 0, 200, 255); // #00C8FF
        private static readonly Color ColourRpm = Color.FromArgb(255, 255, 107, 53);  // #FF6B35
        private static readonly Color ColourTemp = Color.FromArgb(255, 68, 214, 44);  // #44D62C
        private static readonly Color ColourFuel = Color.FromArgb(255, 255, 215, 0);   // #FFD700

        private readonly AppModel _model;
        private int _packetCount;

        public TelemetryPage()
        {
            InitializeComponent();
            _model = App.m_model;

            //                  title           colour       unit    fixedMin  fixedMax
            SpeedChart.Configure("Speed", ColourSpeed, "km/h", 0, 160);
            RpmChart.Configure("RPM", ColourRpm, "rpm", 0, 8000);
            TempChart.Configure("Engine Temp", ColourTemp, "°C", 0, 120);
            FuelChart.Configure("Fuel", ColourFuel, "%", 0, 100);
        }


        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            _model.m_webSocketController.CarDataReceived += OnCarDataReceived;
            PushSnapshot(_model.m_carData);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);
            _model.m_webSocketController.CarDataReceived -= OnCarDataReceived;
        }

        private void OnCarDataReceived(CarData data)
        {
            DispatcherQueue.TryEnqueue(() => PushSnapshot(data));
        }

        private void PushSnapshot(CarData data)
        {
            SpeedChart.AddDataPoint(data.Speed);
            RpmChart.AddDataPoint(data.Rpms);
            TempChart.AddDataPoint(data.EngineTemp);
            FuelChart.AddDataPoint(data.Fuel);

            _packetCount++;

            var running = data.Status;
            StatusDot.Fill = new SolidColorBrush(running ? ColourTemp : Colors.OrangeRed);
            StatusText.Text = running ? "Engine running" : "Engine stopped";
            StatusText.Foreground = new SolidColorBrush(
                running ? Color.FromArgb(255, 120, 220, 100) : Colors.OrangeRed);
            PacketCountText.Text = $"{_packetCount} packets received";
        }
    }
}
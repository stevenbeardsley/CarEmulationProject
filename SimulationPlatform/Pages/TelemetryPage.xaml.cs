using Microsoft.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Models;
using Windows.UI;

namespace SimulationPlatform.Pages
{
    public sealed partial class TelemetryPage : Page
    {
        // ── Palette — mirrors the Python dashboard colours ────────────────────
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

            // Configure chart appearance once — same call site as before,
            // just moved here from the Window constructor.
            SpeedChart.Configure("Speed", ColourSpeed, "km/h");
            RpmChart.Configure("RPM", ColourRpm, "rpm");
            TempChart.Configure("Engine Temp", ColourTemp, "°C");
            FuelChart.Configure("Fuel", ColourFuel, "%");
        }

        // ── Page lifecycle ────────────────────────────────────────────────────
        // Mirrors the OnNavigatedTo / OnNavigatedFrom pattern in TrackPage.

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            // Subscribe when the page becomes visible.
            _model.m_webSocketController.CarDataReceived += OnCarDataReceived;

            // Seed charts with whatever the model already holds so the
            // graphs aren't blank while waiting for the next packet.
            PushSnapshot(_model.m_carData);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);

            // Unsubscribe when navigated away — avoids leaks if the Frame
            // ever navigates elsewhere (and keeps parity with TrackPage).
            _model.m_webSocketController.CarDataReceived -= OnCarDataReceived;
        }

        // ── Data handler ──────────────────────────────────────────────────────

        private void OnCarDataReceived(CarData data)
        {
            // CarDataReceived fires on the WebSocket background thread —
            // marshal onto the UI thread before touching controls.
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

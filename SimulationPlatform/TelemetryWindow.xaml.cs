using Microsoft.UI.Xaml;
using SimulationPlatform.Pages;
using Windows.Graphics;

namespace SimulationPlatform
{
    public sealed partial class TelemetryWindow : Window
    {
        /// <summary>
        /// True while the window has not been closed.
        /// TrackPage checks this before re-activating an existing instance.
        /// </summary>
        public bool IsOpen { get; private set; } = true;

        public TelemetryWindow()
        {
            InitializeComponent();

            Title = "Live Telemetry";
            AppWindow.Resize(new SizeInt32(960, 660));

            // Navigate the hosted Frame to the page that owns all the content
            // and data logic — mirrors the pattern used by MainWindow → TrackPage.
            RootFrame.Navigate(typeof(TelemetryPage));

            Closed += (_, _) => IsOpen = false;
        }
    }
}

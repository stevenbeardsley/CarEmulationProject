using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using SimulationPlatform.Pages;

namespace SimulationPlatform
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
            // Navigate to default page on startup
            ContentFrame.Navigate(typeof(DeployPage));
            NavView.SelectedItem = NavView.MenuItems[0];
        }

        private void NavView_SelectionChanged(NavigationView sender,
                                              NavigationViewSelectionChangedEventArgs args)
        {
            if (args.SelectedItem is NavigationViewItem item)
            {
                switch (item.Tag)
                {
                    case "DeployPage":
                        ContentFrame.Navigate(typeof(DeployPage));
                        break;
                    //case "TrackPage":
                    //    ContentFrame.Navigate(typeof(TrackPage));
                    //    break;
                    //case "LoggingPage":
                    //    ContentFrame.Navigate(typeof(LoggingPage));
                    //    break;
                }
            }
        }
    }
}

using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using SimulationPlatform.Pages;
using SimulationPlatform.Services;

namespace SimulationPlatform
{
    public sealed partial class MainWindow : Window, INotifyPropertyChanged
    {
        private readonly AppModel m_model; // reference to the base model

        private bool m_connected = false;
        public Visibility ConnectedVisibility => m_connected ? Visibility.Visible : Visibility.Collapsed;
        public Visibility DisconnectedVisibility => m_connected ? Visibility.Collapsed : Visibility.Visible;
        public event PropertyChangedEventHandler? PropertyChanged;

        public MainWindow()
        {
            this.InitializeComponent();
            m_model = App.m_model; // Classes are ref type, so this is a ref 

            // Navigate to default page on startup
            ContentFrame.Navigate(typeof(DeployPage));
            NavView.SelectedItem = NavView.MenuItems[0];
            m_model.Connected += OnConnected;
            m_model.Disconnected += OnDisconnected;
        }

        private void OnConnected()
        {
            m_connected = true;
            OnPropertyChanged(nameof(ConnectedVisibility));
            OnPropertyChanged(nameof(DisconnectedVisibility));
        }

        private void OnDisconnected()
        {
            m_connected = false;
            OnPropertyChanged(nameof(ConnectedVisibility));
            OnPropertyChanged(nameof(DisconnectedVisibility));
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
                    case "LoggingPage":
                        ContentFrame.Navigate(typeof(LoggingPage));
                        break;
                    case "TrackPage":
                        ContentFrame.Navigate(typeof(TrackPage));
                        break;
                }
            }
        }
        private void OnPropertyChanged(string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}

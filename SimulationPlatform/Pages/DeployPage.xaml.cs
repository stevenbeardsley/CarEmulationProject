using System.Collections.Specialized;
using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Controllers;

namespace SimulationPlatform.Pages
{
    public sealed partial class DeployPage : Page, INotifyPropertyChanged
    {
        private readonly DeploymentController m_deploymentController = new("Ubuntu");

        private bool m_connected = false;

        public event PropertyChangedEventHandler? PropertyChanged;

        public Visibility ConnectedVisibility => m_connected ? Visibility.Visible : Visibility.Collapsed;
        
        public DeployPage()
        {
            this.InitializeComponent();
            this.DataContext = this;
        }

        private void OnPropertyChanged(string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            App.m_webSocketController.Connected += OnConnected;
            App.m_webSocketController.Disconnected += onDisconnected;

        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            App.m_webSocketController.Connected -= OnConnected;
            App.m_webSocketController.Disconnected -= onDisconnected;
        }


        private async void DeployButton_Click(object sender, RoutedEventArgs e)
        {
            var scriptPath = "/mnt/c/Users/swbea/source/repos/CarEmulationProject/Neon/deploy.sh";  // adjust path
            var test = await m_deploymentController.Deploy(scriptPath);
            // Try and connect 
            await App.m_webSocketController.ConnectAsync("ws://localhost:8080");
            // TOOO, show loaded?

        }
        private void OnConnected()
        {
            _ = DispatcherQueue.TryEnqueue(() =>
            {
                // Disable deploy button 
                m_connected = true;
                OnPropertyChanged(nameof(ConnectedVisibility));
                // Print connected and status is deployed 

            });
        }

        private void onDisconnected()
        {
            _ = DispatcherQueue.TryEnqueue(() =>
            {
                // Disable deploy button 

                // Print connected and status is deployed 

            });
        }


    }
}

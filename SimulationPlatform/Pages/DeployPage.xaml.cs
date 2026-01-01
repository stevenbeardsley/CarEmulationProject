using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Controllers;

namespace SimulationPlatform.Pages
{
    public sealed partial class DeployPage : Page, INotifyPropertyChanged
    {
        private readonly DeploymentController m_deploymentController = new("Ubuntu"); // TODO: Move to the model?

        private readonly AppModel m_model; // reference to the base model
    
        public event PropertyChangedEventHandler? PropertyChanged;

        public Visibility ConnectedVisibility = Visibility.Collapsed;
        public Visibility DeployingVisibility = Visibility.Collapsed;
        public Visibility DeployErrorVisibility = Visibility.Collapsed;
        public DeployPage()
        {
            this.InitializeComponent();
            m_model = App.m_model; // Classes are ref type, so this is a ref 
            DataContext = m_model;
            m_model.Connected += OnConnected;
            m_model.Disconnected += OnDisconnected;
        }

        private void OnConnected()
        {
            DeployingVisibility = Visibility.Collapsed;
            OnPropertyChanged(nameof(DeployingVisibility));
            ConnectedVisibility = Visibility.Visible;
            OnPropertyChanged(nameof(ConnectedVisibility));
        }

        private void OnDisconnected()
        {
            ConnectedVisibility = Visibility.Collapsed;
            OnPropertyChanged(nameof(ConnectedVisibility));
        }

        private void OnPropertyChanged(string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            // Subscribe to message changes 
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
                // TODO 
        }

        private async void DeployButton_Click(object sender, RoutedEventArgs e)
        {
            var scriptPath = "/mnt/c/Users/swbea/source/repos/CarEmulationProject/Neon/deploy.sh";  // adjust path
            DeployingVisibility = Visibility.Visible;
            OnPropertyChanged(nameof(DeployingVisibility));
            var output = await m_deploymentController.Deploy(scriptPath);

            // TODO - Try and just connect if deployment fails 
            
            if (output.ExitCode == 0)
            {
                // Try and connect 
                await m_model.m_webSocketController.ConnectAsync("ws://localhost:8080");
            }
            else
            {
                // Failed to deploy, output error message 
                DeployErrorVisibility = Visibility.Visible;
                OnPropertyChanged(nameof(DeployErrorVisibility));
            }
        }
   


}
}

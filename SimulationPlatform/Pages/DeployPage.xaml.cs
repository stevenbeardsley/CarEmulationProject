using System.Collections.Generic;
using System.ComponentModel;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Controllers;

namespace SimulationPlatform.Pages
{
    public sealed partial class DeployPage : Page, INotifyPropertyChanged
    {
        private readonly Dictionary<string, Brush> m_colourMap =
            new Dictionary<string, Brush>()
            {
                { "Blue", new SolidColorBrush(Colors.Blue) },
                { "Red", new SolidColorBrush(Colors.Red) },
                { "Green", new SolidColorBrush(Colors.Green) },
            };

        public Brush m_selectedCarColour { get; private set; } = new SolidColorBrush(Colors.White); // Default Null colour 

        public string m_transmissionSelection
        {
            get; private set;
        } = string.Empty;
        public string m_engineSelection
        {
            get; private set;
        } = string.Empty;

        private readonly DeploymentController m_deploymentController = new();

        private readonly AppModel m_model; // reference to the base model

        public event PropertyChangedEventHandler? PropertyChanged;

        public Visibility ConnectedVisibility = Visibility.Collapsed;
        public Visibility DeployingVisibility = Visibility.Collapsed;
        public Visibility DeployErrorVisibility = Visibility.Collapsed;
        public Visibility m_selectOptionTextVisibility => m_deployButtonEnabled ? Visibility.Collapsed : Visibility.Visible;

        private bool m_connected = false;
        private bool m_colourChosen = false;
        private bool m_engineTypeChosen = false; 
        private bool m_transmissionTypeChosen = false;
        public bool m_deployButtonEnabled => m_colourChosen && 
            m_transmissionTypeChosen &&
            m_engineTypeChosen &&
            DeployingVisibility != Visibility.Visible &&
            DeployErrorVisibility != Visibility.Visible &&
            !m_connected;

        public bool m_selectionEnabled =>
            !m_connected;
        public bool m_undeployButtonEnabled => m_connected;

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
            DispatcherQueue.TryEnqueue(() =>
            {
                m_connected = true; 

                OnPropertyChanged(nameof(ConnectedVisibility));

                OnPropertyChanged(nameof(m_deployButtonEnabled));
                OnPropertyChanged(nameof(m_undeployButtonEnabled));
                OnPropertyChanged(nameof(m_selectOptionTextVisibility));
                DeployingVisibility = Visibility.Collapsed;
                OnPropertyChanged(nameof(DeployingVisibility));
                ConnectedVisibility = Visibility.Visible;
                OnPropertyChanged(nameof(ConnectedVisibility));
                OnPropertyChanged(nameof(m_selectionEnabled));

            });
        }

        private void OnDisconnected()
        {
            DispatcherQueue.TryEnqueue(() =>
            {
                m_connected = false;

                ConnectedVisibility = Visibility.Collapsed;
                OnPropertyChanged(nameof(ConnectedVisibility));

                OnPropertyChanged(nameof(m_deployButtonEnabled));
                OnPropertyChanged(nameof(m_undeployButtonEnabled));
                OnPropertyChanged(nameof(m_selectOptionTextVisibility));
                OnPropertyChanged(nameof(m_selectionEnabled));
            });
        }

        private void OnPropertyChanged(string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            if (App.m_model.isConnected())
            {
                OnConnected();
            }
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            m_model.Connected -= OnConnected;
            m_model.Disconnected -= OnDisconnected;
        }

        private (string, string) getOptionIds()
        {
            var transmissionId = string.Empty;
            switch (m_transmissionSelection)
            {
                case "5-speed":
                    transmissionId = "transmission_5spd";
                    break;
                case "7-speed":
                    transmissionId = "transmission_7spd";
                    break;
            }
            var engineId = string.Empty;
            switch (m_engineSelection)
            {
                case "1.0L":
                    engineId = "engine_1L";
                    break;
                case "2.0L":
                    engineId = "engine_2L";
                    break;
            }

            return (transmissionId, engineId);
        }

        private async void UndeployButton_Click(object sender, RoutedEventArgs e)
        {
            var scriptPath = "/mnt/c/Users/swbea/source/repos/CarEmulationProject/Neon/clean_docker.sh";
            var output = await m_deploymentController.Undeploy(scriptPath);
            // Depending on how your model is architected, undeploying here should 
            // trigger the m_model.Disconnected event automatically. If it doesn't, 
            // you might need to manually tell the model to disconnect its websocket.
            if (output.ExitCode == 0)
            {
                await m_model.m_webSocketController.DisconnectAsync();
            }
        }

        private async void DeployButton_Click(object sender, RoutedEventArgs e)
        {
            var scriptPath = "/mnt/c/Users/swbea/source/repos/CarEmulationProject/Neon/deploy.sh";  
            DeployingVisibility = Visibility.Visible;
            OnPropertyChanged(nameof(DeployingVisibility));
            OnPropertyChanged(nameof(m_deployButtonEnabled));
            OnPropertyChanged(nameof(m_selectOptionTextVisibility));
            var (transmissionId, engineId) = getOptionIds(); 
            var output = await m_deploymentController.Deploy(scriptPath, transmissionId, engineId);

            if (output.ExitCode == 0)
            {
                // Try and connect 
                await m_model.m_webSocketController.ConnectAsync("ws://localhost:8080");
            }
            else
            {
                // Failed to deploy, output error message 
                DeployingVisibility = Visibility.Collapsed;
                DeployErrorVisibility = Visibility.Visible;

                OnPropertyChanged(nameof(DeployingVisibility));
                OnPropertyChanged(nameof(DeployErrorVisibility));
                OnPropertyChanged(nameof(m_deployButtonEnabled));
                OnPropertyChanged(nameof(m_selectOptionTextVisibility));
            }
        }
        private void ColourComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var selected = (ComboBoxItem)((ComboBox)sender).SelectedItem;
            switch (selected.Content.ToString())
            {
                case "Blue":
                    m_selectedCarColour = new SolidColorBrush(Colors.Blue);
                    break;

                case "Red":
                    m_selectedCarColour = new SolidColorBrush(Colors.Red);
                    break;

                case "Green":
                    m_selectedCarColour = new SolidColorBrush(Colors.Green);
                    break;
            }
            App.m_model.CarConfig.Colour = m_selectedCarColour;
            m_colourChosen = true;
            OnPropertyChanged(nameof(m_deployButtonEnabled));
            OnPropertyChanged(nameof(m_selectOptionTextVisibility));
        }

        private void TransmissionComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var selected = (ComboBoxItem)((ComboBox)sender).SelectedItem;
            m_transmissionSelection = selected.Content.ToString();
            switch (selected.Content.ToString())
            {
                case "5-speed":
                    App.m_model.CarConfig.GearsCount = 5;
                    break;
                case "7-speed":
                    App.m_model.CarConfig.GearsCount = 7;
                    break;
            }
            m_transmissionTypeChosen = true;
            OnPropertyChanged(nameof(m_deployButtonEnabled));
            OnPropertyChanged(nameof(m_selectOptionTextVisibility));
        }

        private void EngineComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var selected = (ComboBoxItem)((ComboBox)sender).SelectedItem;
            m_engineSelection = selected.Content.ToString();
            m_engineTypeChosen = true;
            OnPropertyChanged(nameof(m_deployButtonEnabled));
            OnPropertyChanged(nameof(m_selectOptionTextVisibility));
        }
    }
}
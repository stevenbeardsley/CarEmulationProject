using System.Collections.Generic;
using System.ComponentModel;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using SimulationPlatform.Controllers;
using Windows.UI;

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
        
        private readonly DeploymentController m_deploymentController = new("Ubuntu"); // TODO: Move to the model?

        private readonly AppModel m_model; // reference to the base model

        public event PropertyChangedEventHandler? PropertyChanged;

        public Visibility ConnectedVisibility = Visibility.Collapsed;
        public Visibility DeployingVisibility = Visibility.Collapsed;
        public Visibility DeployErrorVisibility = Visibility.Collapsed;
        public Visibility m_selectOptionTextVisibility => m_deployButtonEnabled ? Visibility.Collapsed : Visibility.Visible;

        private bool m_colourChosen = false;
        private bool m_engineTypeChosen = false; // TODO: When engine configurations are implemented.
        private bool m_transmissionTypeChosen = false;
        public bool m_deployButtonEnabled => m_colourChosen && 
            m_transmissionTypeChosen &&
            DeployingVisibility != Visibility.Visible &&
            DeployErrorVisibility != Visibility.Visible;
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
            OnPropertyChanged(nameof(m_deployButtonEnabled));
            OnPropertyChanged(nameof(m_selectOptionTextVisibility));
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
                OnPropertyChanged(nameof(m_deployButtonEnabled));
                OnPropertyChanged(nameof(m_selectOptionTextVisibility));
            }
        }
        private void ColourComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e) // TODO: Could just do this on deploy click really
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
    }
}
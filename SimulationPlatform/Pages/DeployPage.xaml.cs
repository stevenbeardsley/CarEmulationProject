using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using SimulationPlatform.Controllers;
using SimulationPlatform.Models;

namespace SimulationPlatform.Pages
{
    public sealed partial class DeployPage : Page
    {
        private readonly DeploymentController m_deploymentController = new("Ubuntu");
        private readonly WebSocketController _controller = new();

        public DeployPage()
        {
            this.InitializeComponent();
            // Set a simple title in the TextBlock defined in XAML
            _controller.Connected += OnConnected;
            _controller.Disconnected += OnDisconnected;
            _controller.LogMessage += OnLogMessage;
            _controller.StatusReceived += OnStatusReceived;
        }

        private async void DeployButton_Click(object sender, RoutedEventArgs e)
        {
            var scriptPath = "/mnt/c/Users/swbea/source/repos/CarEmulationProject/Neon/deploy.sh";  // adjust path
            var test = await m_deploymentController.Deploy(scriptPath);

        }
        private void OnConnected()
        {
            _ = DispatcherQueue.TryEnqueue(() =>
            {
                ConnectionStatus.Text = "Connected";
                ConnectionStatus.Foreground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Microsoft.UI.Colors.Green);
            });
        }

        private void OnDisconnected()
        {
            _ = DispatcherQueue.TryEnqueue(() =>
            {
                ConnectionStatus.Text = "Disconnected";
                ConnectionStatus.Foreground = new Microsoft.UI.Xaml.Media.SolidColorBrush(Microsoft.UI.Colors.Red);
            });
        }

        private void OnLogMessage(string msg)
        {
            _ = DispatcherQueue.TryEnqueue(() =>
            {
                LogOutput.Text += $"{DateTime.Now:HH:mm:ss} - {msg}\n";
            });
        }

        private void OnStatusReceived(DashboardMessage msg)
        {
            _ = DispatcherQueue.TryEnqueue(() =>
            {
                StatusText.Text = msg.Status;
                PidText.Text = msg.Pid.ToString();
            });
        }
        private async void ConnectButton_Click(object sender, RoutedEventArgs e)
        {
            await _controller.ConnectAsync("ws://localhost:8080");
        }
        private async void DisconnectButton_Click(object sender, RoutedEventArgs e)
        {
            await _controller.DisconnectAsync();
        }
    }
}

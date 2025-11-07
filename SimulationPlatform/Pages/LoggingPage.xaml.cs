using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using SimulationPlatform.Controllers;
using SimulationPlatform.Models;

namespace SimulationPlatform.Pages
{
    public sealed partial class LoggingPage : Page
    {
        private readonly WebSocketController _controller = new();

        public LoggingPage()
        {
            this.InitializeComponent();
            // Set a simple title in the TextBlock defined in XAML
            _controller.Connected += OnConnected;
            _controller.Disconnected += OnDisconnected;
            _controller.LogMessage += OnLogMessage;
            _controller.StatusReceived += OnStatusReceived;
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
                StatusText.Text = msg.m_speed;
                PidText.Text = msg.m_status.ToString();
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
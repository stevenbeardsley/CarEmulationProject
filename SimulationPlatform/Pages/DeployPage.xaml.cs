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

        public DeployPage()
        {
            this.InitializeComponent();
            // Set a simple title in the TextBlock defined in XAML
        }

        private async void DeployButton_Click(object sender, RoutedEventArgs e)
        {
            var scriptPath = "/mnt/c/Users/swbea/source/repos/CarEmulationProject/Neon/deploy.sh";  // adjust path
            var test = await m_deploymentController.Deploy(scriptPath);

        }
    }
}

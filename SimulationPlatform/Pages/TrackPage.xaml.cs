using System;
using System.Runtime.CompilerServices;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;

namespace SimulationPlatform.Pages
{
    public sealed partial class TrackPage : Page
    {

        private readonly AppModel m_model;

        public TrackPage()
        {
            InitializeComponent();
            m_model = App.m_model; 
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            ;
        }


        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            ;
        }
        private async void GearShiftDown_Click(object sender, RoutedEventArgs e)
        {
            
        }

        private async void GearShiftUp_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await m_model.VehicleController.ShiftUpAsync();
            }
            catch (Exception ex)
            {
                // TODO: log or surface error
                System.Diagnostics.Debug.WriteLine(ex.Message);
            }
        }

    }
}
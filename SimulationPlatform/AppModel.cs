using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Transactions;
using SimulationPlatform.Controllers;
using SimulationPlatform.Models;

namespace SimulationPlatform
{

public class AppModel : INotifyPropertyChanged
{
        private bool m_connected = false; // If the VSP is connected to NEON
        public event Action? Connected;
        public event Action? Disconnected;

        public CarData m_carData; // Current car data being fed back 
        public CarConfig CarConfig { get; } = new CarConfig(); // user config
        public event PropertyChangedEventHandler PropertyChanged;
        public WebSocketController m_webSocketController = new WebSocketController(isCommandSocket: false);
        public VehicleController VehicleController
        {
            get;
        }
        private readonly HttpClient m_commandSender;

        public AppModel()
        {
            m_carData = new CarData();
            m_webSocketController.Connected += OnConnected;
            m_webSocketController.Disconnected += OnDisconnected;
            m_webSocketController.CarDataReceived += OnCarDataReceived;

            m_commandSender = new HttpClient("localhost", 8081);
            VehicleController = new VehicleController(m_commandSender);
        }

        private void OnConnected()
        {
            m_connected = true;
            Connected?.Invoke();
        }

        private void OnDisconnected()
        {
            m_connected = false;
            Disconnected?.Invoke();
        }

        public void OnCarDataReceived(CarData newMessage)
        {
            if (newMessage != null)
            {
                m_carData = newMessage;
            }
        }

        public bool isConnected()
        {
            return m_connected;
        }

        protected void OnPropertyChanged([CallerMemberName] string name = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

    }

}
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
        public event PropertyChangedEventHandler PropertyChanged;
        public WebSocketController m_webSocketController = new WebSocketController(isCommandSocket: false);

        public AppModel()
        {
            m_carData = new CarData();
            m_webSocketController.Connected += OnConnected;
            m_webSocketController.Disconnected += OnDisconnected;
            m_webSocketController.CarDataReceived += OnCarDataReceived;
        }
        // Standard event boilerplate

        public CarData GetCarData()
        {
            return m_carData;
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

        protected void OnPropertyChanged([CallerMemberName] string name = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

    }

}
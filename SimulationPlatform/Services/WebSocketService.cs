using System;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using SimulationPlatform.Models;
using Newtonsoft.Json.Linq;

namespace SimulationPlatform.Services
{
    public class WebSocketService
    {
        private readonly ClientWebSocket _ws = new ClientWebSocket();
        private DashboardModel _latestStatus = new DashboardModel();
        private bool _connected = false;


        public async Task ConnectAsync(string uri)
        {
            if (_connected)
                return;

            try
            {
                Console.WriteLine($"Connecting to {uri}...");
                await _ws.ConnectAsync(new Uri(uri), CancellationToken.None);
                _connected = true;
                Console.WriteLine("Connected!");

                _ = Task.Run(async () => await ListenAsync()); // start background listener
            }
            catch (Exception ex)
            {
                Console.WriteLine($"WebSocket connection failed: {ex.Message}");
            }
        }

        private async Task ListenAsync()
        {
            var buffer = new byte[1024];

            while (_ws.State == WebSocketState.Open)
            {
                var result = await _ws.ReceiveAsync(new ArraySegment<byte>(buffer), CancellationToken.None);
                var message = Encoding.UTF8.GetString(buffer, 0, result.Count);

                try
                {
                    var json = JObject.Parse(message);
                    _latestStatus = new DashboardModel
                    {
                        m_speed = json["speed"]?.ToString() ?? "0",
                        m_status = json["status"]?.ToString() ?? "disconnected",
                        m_timestamp = DateTime.UtcNow
                    };

                    Console.WriteLine($"Updated status: {_latestStatus.m_status}, status: {_latestStatus.m_status}");
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to parse JSON: {ex.Message}");
                }

                if (result.MessageType == WebSocketMessageType.Close)
                {
                    Console.WriteLine("Server closed connection.");
                    await _ws.CloseAsync(WebSocketCloseStatus.NormalClosure, "Closing", CancellationToken.None);
                    _connected = false;
                    break;
                }
            }
        }
    }
}

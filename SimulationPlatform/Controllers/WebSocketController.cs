using System;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using SimulationPlatform.Models;

namespace SimulationPlatform.Controllers
{
    public class WebSocketController : IDisposable
    {
        private ClientWebSocket? _ws;
        private CancellationTokenSource? _cts;
        private readonly bool _isCommandSocket;

        public event Action? Connected;
        public event Action? Disconnected;
        public event Action<string>? LogMessage;
        public event Action<CarData>? CarDataReceived;

        public WebSocketController(bool isCommandSocket = false)
        {
            _isCommandSocket = isCommandSocket;
        }

        public async Task ConnectAsync(string host = "ws://localhost", int port = 8080)
        {
            if (_ws?.State == WebSocketState.Open)
            {
                LogMessage?.Invoke("Already connected.");
                return;
            }

            _ws = new ClientWebSocket();
            _cts = new CancellationTokenSource();

            string path = _isCommandSocket ? "carCommands" : "carData";

            // ✅ Fix: only append port if host doesn’t already include one
            string baseUrl = host.Contains(":") && !host.EndsWith("localhost")
                ? host
                : $"{host}:{port}";

            string url = $"{baseUrl}/{path}";

            try
            {
                LogMessage?.Invoke($"🔌 Connecting to {url}...");
                await _ws.ConnectAsync(new Uri(url), _cts.Token);
                Connected?.Invoke();
                LogMessage?.Invoke($"✅ Connected to {_isCommandSocket switch { true => "command", false => "data" }} socket.");

                if (!_isCommandSocket)
                    _ = Task.Run(() => ReceiveLoopAsync(_cts.Token));
            }
            catch (Exception ex)
            {
                LogMessage?.Invoke($"❌ Connection failed: {ex.Message}");
                Disconnected?.Invoke();
            }
        }


        private async Task ReceiveLoopAsync(CancellationToken token)
        {
            var buffer = new byte[8192];

            try
            {
                while (_ws != null && _ws.State == WebSocketState.Open && !token.IsCancellationRequested)
                {
                    var result = await _ws.ReceiveAsync(buffer, token);

                    if (result.MessageType == WebSocketMessageType.Close)
                    {
                        await _ws.CloseAsync(WebSocketCloseStatus.NormalClosure, "Server closed connection", token);
                        LogMessage?.Invoke("🔒 Connection closed by server.");
                        Disconnected?.Invoke();
                        return;
                    }

                    string message = Encoding.UTF8.GetString(buffer, 0, result.Count);
                    LogMessage?.Invoke($"📩 Received: {message}");

                    try
                    {
                        var msg = DashboardMessage.FromJson(message);
                        if (msg != null)
                        {
                            var carData = msg.ToCarData();
                            CarDataReceived?.Invoke(carData);
                        }
                        else
                        {
                            LogMessage?.Invoke("⚠️ Invalid JSON received.");
                        }
                    }
                    catch (Exception ex)
                    {
                        LogMessage?.Invoke($"⚠️ JSON parse error: {ex.Message}");
                    }
                }
            }
            catch (Exception ex)
            {
                LogMessage?.Invoke($"⚠️ Receive loop ended: {ex.Message}");
                Disconnected?.Invoke();
            }
        }

        public async Task SendCommandAsync(string command)
        {
            if (!_isCommandSocket)
            {
                LogMessage?.Invoke("⚠️ This WebSocket is for receiving data, not sending commands.");
                return;
            }

            if (_ws == null || _ws.State != WebSocketState.Open)
            {
                LogMessage?.Invoke("⚠️ Not connected to server.");
                return;
            }

            try
            {
                var bytes = Encoding.UTF8.GetBytes(command);
                await _ws.SendAsync(bytes, WebSocketMessageType.Text, true, CancellationToken.None);
                LogMessage?.Invoke($"🚀 Sent command: {command}");
            }
            catch (Exception ex)
            {
                LogMessage?.Invoke($"❌ Send failed: {ex.Message}");
            }
        }

        public async Task DisconnectAsync()
        {
            try
            {
                if (_ws != null && _ws.State == WebSocketState.Open)
                {
                    _cts?.Cancel();
                    await _ws.CloseAsync(WebSocketCloseStatus.NormalClosure, "Client disconnect", CancellationToken.None);
                    LogMessage?.Invoke("🔌 Disconnected.");
                    Disconnected?.Invoke();
                }
            }
            catch (Exception ex)
            {
                LogMessage?.Invoke($"⚠️ Error during disconnect: {ex.Message}");
            }
        }

        public void Dispose()
        {
            _cts?.Cancel();
            _ws?.Dispose();
        }
    }
}

using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace SimulationPlatform
{
    public class HttpClient : IDisposable, ICommandSender
    {
        private readonly System.Net.Http.HttpClient m_httpClient;
        private readonly string m_baseUrl;

        public HttpClient(string host = "localhost", int port = 8081)
        {
            m_baseUrl = $"http://{host}:{port}";

            m_httpClient = new System.Net.Http.HttpClient
            {
                Timeout = TimeSpan.FromSeconds(2)
            };
        }

        /// <summary>
        /// Sends a command as JSON with a single field and value.
        /// Example JSON: { "gear": 3 }
        /// </summary>
        public async Task<bool> SendAsync(
            string endpoint,
            string fieldName,
            object value)
        {
            var payload = new
            {
                // dynamic JSON field name
                // this will be serialized correctly below
            };

            var json = JsonSerializer.Serialize(
                new Dictionary<string, object>
                {
                    { fieldName, value }
                }
            );

            using var content = new StringContent(
                json,
                Encoding.UTF8,
                "application/json"
            );

            var response = await m_httpClient.PostAsync(
                $"{m_baseUrl}/{endpoint}",
                content
            );

            return response.IsSuccessStatusCode;
        }

        public void Dispose()
        {
            m_httpClient.Dispose();
        }
    }
}

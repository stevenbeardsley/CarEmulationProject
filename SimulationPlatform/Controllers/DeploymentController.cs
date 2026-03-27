using System;
using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;

namespace SimulationPlatform.Controllers
{
    public class DeploymentController
    {
        private readonly string _distroName;

        public DeploymentController(string distroName = "Ubuntu")
        {
            _distroName = distroName;
        }

        public async Task<(int ExitCode, string Output, string Error)> Undeploy(string scriptPath)
        {
            return await RunWslScript(scriptPath);
        }

        public async Task<(int ExitCode, string Output, string Error)> Deploy(string scriptPath,
            string transmissionId, string engineId)
        {
            var config = CreateConfig(engineId, transmissionId);
            var json = JsonSerializer.Serialize(config, new JsonSerializerOptions { WriteIndented = true });

            var deployDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "SimulationPlatform");
            Directory.CreateDirectory(deployDir);
            var configPath = Path.Combine(deployDir, "config.json");
            await File.WriteAllTextAsync(configPath, json);

            return await RunWslScript(scriptPath, configPath);
        }

        private async Task<(int ExitCode, string Output, string Error)> RunWslScript(string scriptPath, string args = "")
        {
            // Escape single quotes for bash
            var safeScriptPath = scriptPath.Replace("'", "'\\''");
            var bashCmd = $"'{safeScriptPath}' {args}";
            var wslArgs = $"-d {_distroName} -- bash -lc \"{bashCmd}\"";

            var psi = new ProcessStartInfo
            {
                FileName = "wsl.exe",
                Arguments = wslArgs,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = new Process { StartInfo = psi };
            process.Start();

            var outputTask = process.StandardOutput.ReadToEndAsync();
            var errorTask = process.StandardError.ReadToEndAsync();

            await process.WaitForExitAsync();
            await Task.Delay(4000);
            return (process.ExitCode, await outputTask, await errorTask);
        }

        private static object CreateConfig(string engineId, string transmissionId)
        {
            // TODO: Move these to dedicated classes/records later
            var engine = engineId switch
            {
                "engine_1L" => new { id = "engine_1L", displacement_l = 1.0, idle_rpm = 850, max_rpm = 6000, max_torque_nm = 110 },
                "engine_2L" => new { id = "engine_2L", displacement_l = 2.0, idle_rpm = 900, max_rpm = 6500, max_torque_nm = 190 },
                _ => throw new ArgumentException($"Unknown engine: {engineId}")
            };

            var transmission = transmissionId switch
            {
                "transmission_5spd" => new { id = "transmission_5spd", gears = 5, gear_ratios = new[] { 3.91, 2.14, 1.36, 1.03, 0.84 }, reverse_ratio = -3.54 },
                "transmission_7spd" => new { id = "transmission_7spd", gears = 7, gear_ratios = new[] { 4.38, 2.86, 1.92, 1.37, 1.00, 0.82, 0.64 }, reverse_ratio = -3.20 },
                _ => throw new ArgumentException($"Unknown transmission: {transmissionId}")
            };

            return new { engine, transmission };
        }
    }
}
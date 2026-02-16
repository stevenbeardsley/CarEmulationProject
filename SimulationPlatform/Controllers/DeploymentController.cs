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

        public async Task<(int ExitCode, string Output, string Error)> Deploy(string scriptPath, 
            string transmissionId, string engineId)
        {
            // Create config file 
            var config = CreateConfig(engineId, transmissionId);

            var jsonOptions = new JsonSerializerOptions
            {
                WriteIndented = true
            };

            var json = JsonSerializer.Serialize(config, jsonOptions);

            var scriptDirectory = Path.GetDirectoryName(scriptPath)
                ?? throw new Exception("Invalid script path");


            var deployDir = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.CommonDocuments), // C:\Users\Public\Documents
                "SimulationPlatform");

            Directory.CreateDirectory(deployDir);

            var configPath = Path.Combine(deployDir, "config.json");
            await File.WriteAllTextAsync(configPath, json);

            Debug.WriteLine($"Config written to: {configPath}");

            var exists = File.Exists(configPath);
            var size = exists ? new FileInfo(configPath).Length : -1;

            Debug.WriteLine($"Config written to: {configPath}");
            Debug.WriteLine($"Config exists: {exists}, size: {size} bytes");
            // Quote the script path safely for bash -c
            var bashCmd = $"'{scriptPath.Replace("'", "'\\''")}'"; // single-quote safe
            var arguments = $"-d {_distroName} -- bash -lc {bashCmd}";


            var psi = new ProcessStartInfo
            {
                FileName = "wsl.exe",
                Arguments = arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var process = new Process { StartInfo = psi };
            process.Start();

            // Read both streams concurrently to avoid deadlock
            var outputTask = process.StandardOutput.ReadToEndAsync();
            var errorTask = process.StandardError.ReadToEndAsync();

            await process.WaitForExitAsync();

            var output = await outputTask;
            var error = await errorTask;

            await Task.Delay(4000);
            return (process.ExitCode, output, error); 
        }


        // Creates the config object based on selected options TODO: Move the info to a .xml file?
        private static object CreateConfig(string engineId, string transmissionId)
        {
            var engine = engineId switch
            {
                "engine_1L" => new
                {
                    id = "engine_1L",
                    displacement_l = 1.0,
                    idle_rpm = 850,
                    max_rpm = 6000,
                    max_torque_nm = 110
                },

                "engine_2L" => new
                {
                    id = "engine_2L",
                    displacement_l = 2.0,
                    idle_rpm = 900,
                    max_rpm = 6500,
                    max_torque_nm = 190
                },

                _ => throw new Exception($"Unknown engine: {engineId}")
            };

            var transmission = transmissionId switch
            {
                "transmission_5spd" => new
                {
                    id = "transmission_5spd",
                    gears = 5,
                    final_drive = 3.90,
                    gear_ratios = new[] { 3.91, 2.14, 1.36, 1.03, 0.84 },
                    reverse_ratio = -3.54
                },

                "transmission_7spd" => new
                {
                    id = "transmission_7spd",
                    gears = 7,
                    final_drive = 3.42,
                    gear_ratios = new[] { 4.38, 2.86, 1.92, 1.37, 1.00, 0.82, 0.64 },
                    reverse_ratio = -3.20
                },

                _ => throw new Exception($"Unknown transmission: {transmissionId}")
            };

            return new
            {
                engine,
                transmission
            };
        }
    }

}

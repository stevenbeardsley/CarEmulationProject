using System;
using System.Diagnostics;
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

        public async Task<(int ExitCode, string Output, string Error)> Deploy(string scriptPath)
        {
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

            return (process.ExitCode, output, error); // TODO: Needs breakpoint here for some reason, threading issue?
        }

    }
}

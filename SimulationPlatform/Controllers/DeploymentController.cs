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
            // TODO: Create the config.json

            var arguments = $"-d {_distroName} bash -c \"{scriptPath}\"";

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

            var output = await process.StandardOutput.ReadToEndAsync();
            var error = await process.StandardError.ReadToEndAsync();

            await process.WaitForExitAsync();

            return (process.ExitCode, output, error);
        }
    }
}

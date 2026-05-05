import os
import subprocess
import platform
import shutil
import argparse

# --- Configuration ---
BUILD_DIR = "build_coverage"
HTML_DIR = "coverage_html"

def run_command(cmd, cwd=None):
    print(f"\n>>> Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, text=True, cwd=cwd)
    if result.returncode != 0:
        print(f"[-] Command failed with exit code {result.returncode}")
        exit(1)

def main():
    parser = argparse.ArgumentParser(description="Generate gcov coverage report for Neon shared library.")
    parser.add_argument("--rebuild",  action="store_true", help="Force a clean rebuild.")
    parser.add_argument("--diagnose", action="store_true", help="Run gcovr with no filters to reveal raw paths.")
    args = parser.parse_args()

    if platform.system() == "Windows":
        msys2_path = r"C:\msys64\ucrt64\bin"
        if not os.path.exists(msys2_path):
            print(f"[-] Error: Could not find MSYS2 at {msys2_path}")
            exit(1)
        os.environ["PATH"] = msys2_path + os.pathsep + os.environ["PATH"]

    if args.rebuild:
        if os.path.exists(BUILD_DIR):
            print(f"[--rebuild] Cleaning: {BUILD_DIR}")
            shutil.rmtree(BUILD_DIR)
        cmake_cmd = ["cmake", "-B", BUILD_DIR, "-S", ".", "-DENABLE_COVERAGE=ON"]
        if platform.system() == "Windows":
            cmake_cmd.extend(["-G", "MinGW Makefiles"])
        run_command(cmake_cmd)
        run_command(["cmake", "--build", BUILD_DIR])
    else:
        if not os.path.exists(BUILD_DIR):
            print(f"[-] Build directory '{BUILD_DIR}' not found. Run with --rebuild first.")
            exit(1)
        print(f"[*] Skipping rebuild. Pass --rebuild to force a clean build.")

    print("\n--- Running Unit Tests ---")
    run_command(["ctest", "--output-on-failure"], cwd=BUILD_DIR)

    root_abs  = os.path.abspath(".").replace("\\", "/")
    build_abs = os.path.abspath(BUILD_DIR).replace("\\", "/")

    print("\n--- Generating HTML Coverage Report ---")
    os.makedirs(HTML_DIR, exist_ok=True)
    html_output = os.path.join(HTML_DIR, "neon_test_cov_report.html").replace("\\", "/")

    if args.diagnose:
        print("\n[DIAGNOSE MODE] No filters — shows all raw paths gcovr finds.\n")
        subprocess.run([
            "gcovr",
            "--root",             root_abs,
            "--object-directory", build_abs,
            "--verbose",
        ], text=True)
        return

    html_cmd = [
        "gcovr",
        "--root",             root_abs,
        "--object-directory", build_abs,

        "--filter", ".*shared.*",

        # Excludes ensure nothing outside shared/ sneaks in
        "--exclude", ".*/tests/.*",
        "--exclude", ".*\\\\tests\\\\.*",   # catches backslash variant
        "--exclude", ".*/tcm/.*",
        "--exclude", ".*\\\\tcm\\\\.*",
        "--exclude", ".*/ecm/.*",
        "--exclude", ".*\\\\ecm\\\\.*",
        "--exclude", ".*/dashboard/.*",
        "--exclude", ".*\\\\dashboard\\\\.*",
        "--exclude", ".*/abs/.*",
        "--exclude", ".*/msys64/.*",
        "--exclude", ".*\\\\msys64\\\\.*",
        "--exclude", ".*\\\\abs\\\\.*",
        "--exclude", ".*/build_coverage/.*",
        "--exclude", ".*\\\\build_coverage\\\\.*",
        "--exclude", ".*/_deps/.*",
        "--exclude", ".*/vcpkg_installed/.*",

        "--exclude-unreachable-branches",
        "--exclude-throw-branches",

        "--html",
        "--html-details",
        "--html-title", "Neon Shared Library Coverage Report",
        "-o", html_output,
    ]
    run_command(html_cmd)

    print(f"\n[+] Report at: {html_output}")
    if platform.system() == "Windows":
        os.startfile(html_output.replace("/", "\\"))

if __name__ == "__main__":
    main()
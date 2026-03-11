import os
import subprocess
import platform
import shutil

# --- Configuration ---
BUILD_DIR = "build_coverage"
HTML_DIR = "coverage_html"
# ---------------------

def run_command(cmd, cwd=None):
    print(f"\n>>> Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, text=True, cwd=cwd)
    if result.returncode != 0:
        print(f"[-] Command failed with exit code {result.returncode}")
        exit(1)

def main():
    # 1. Safely inject MSYS2 into the path ONLY for this script (Windows)
    if platform.system() == "Windows":
        msys2_path = r"C:\msys64\ucrt64\bin"
        if not os.path.exists(msys2_path):
            print(f"[-] Error: Could not find MSYS2 at {msys2_path}")
            exit(1)
        os.environ["PATH"] = msys2_path + os.pathsep + os.environ["PATH"]

    if os.path.exists(BUILD_DIR):
        print(f"Cleaning old build directory: {BUILD_DIR}")
        shutil.rmtree(BUILD_DIR)

    # 2. Configure CMake
    cmake_cmd = ["cmake", "-B", BUILD_DIR, "-S", ".", "-DENABLE_COVERAGE=ON"]
    if platform.system() == "Windows":
        cmake_cmd.extend(["-G", "MinGW Makefiles"])
    run_command(cmake_cmd)

    # 3. Build the project
    run_command(["cmake", "--build", BUILD_DIR])

    # 4. Run ONLY the Unit Tests
    print("\n--- Running Unit Tests ---")
    
    # Method A: The CMake Native Way (Highly Recommended)
    # If you use add_test() or gtest_discover_tests() in your CMakeLists, CTest handles everything cleanly.
    print("Executing tests via CTest...")
    run_command(["ctest", "--output-on-failure"], cwd=BUILD_DIR)
    

    # 5. Generate the Filtered HTML coverage report
    print("\n--- Generating HTML Coverage Report ---")
    os.makedirs(HTML_DIR, exist_ok=True)
    html_output = os.path.join(HTML_DIR, "index.html")
    html_output = os.path.join(HTML_DIR, "neon_test_cov_report.html")
    html_cmd = [
        "gcovr", 
        "--root", ".", 
        "--object-directory", BUILD_DIR,
        "--html", 
        "--html-details", 
        "--html-title", "Neon Test Coverage Report",
        "-o", html_output,
        
        # --- FILTERS ---
        # Exclude the tests directory itself (we don't care about test file coverage)
        "--exclude", ".*/tests/.*",
        
        # Exclude GoogleTest source code (if it downloads via FetchContent/vcpkg)
        "--exclude", ".*/_deps/.*",
        "--exclude", ".*/vcpkg_installed/.*",
        
        # Exclude standard library/compiler files
        "--exclude-unreachable-branches",
        "--exclude-throw-branches"
    ]
    run_command(html_cmd)
    
    print(f"\n[+] Success! Coverage report generated at: {html_output}")
    if platform.system() == "Windows":
        os.startfile(html_output)

if __name__ == "__main__":
    main()
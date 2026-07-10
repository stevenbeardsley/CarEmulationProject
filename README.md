# Simulated Automotive Software (SAS)

> Simulated Automotive Software (SAS) builds off the new wave of digital twin systems and virtual environments. The purpose of SAS is to create a test-bed for the development of new automotive components, allowing them to be developed and integrated into a synthetic environment, and networked and integrated with a variety of virtual and real components.

This project presents the design and implementation of Simulated Automotive Software (SAS), a modular digital twin of an automotive system built on industry-standard languages, protocols and tooling. The project comprises two systems: NEON, a collection of independently running Electronic Control Unit (ECU) simulations containerised using Docker, and the Vehicle Simulation Platform (VSP), a Windows desktop application used to configure, deploy and monitor the simulation in real time.

NEON consists of three containerised C++ components: an Engine Control Module, a Transmission Control Module, and a Dashboard. These are networked together using an adapted implementation of the Controller Area Network (CAN) protocol, serialised at the bit level and transmitted over UDP. The Engine Control Module simulates torque generation, fuel consumption, and thermal dynamics using physically grounded mathematical models. The Dashboard bridges the NEON network and the VSP through HTTP and WebSockets, while also routing commands inbound from the VSP as CAN messages.

The VSP is implemented using the .NET framework following a Model-View-Controller architecture, providing real-time telemetry graphs and vehicle control inputs, allowing the user to configure, control and monitor the NEON system.

---

## Features

- [ ] Bit-level serialisation
- [ ] Emulation of the industry-standard layer 2 Controller Area Network protocol, adapted over UDP
- [ ] Real-time, thread-safe logging library
- [ ] Modular, containerised architecture spanning multiple operating systems and build systems
- [ ] A supporting Windows desktop application (VSP) for configuration, deployment and monitoring
- [ ] Physically grounded engine simulation, covering torque generation, fuel consumption and thermal dynamics

---

## Testing

### Unit Tests & Test Coverage

The NEON libraries (CAN, Bit Parser, Config, Logging) were tested using **Google Test**, with each library covered by an extensive set of test cases targeting both expected behaviour and edge cases. Test coverage was measured using **gcov**, with a Python script used to automate the build, test, and report generation pipeline, producing an HTML coverage report for review (see `E.3` in the original dissertation for the script).

This process achieved:

- **91.1%** line coverage across NEON library code
- **96.5%** function coverage across NEON library code

These reports were generated and reviewed at multiple points during development, helping identify missed test cases early and ensuring confidence in the correctness of shared, reused library code. The VSP itself was not unit tested directly, as its behaviour is instead covered by the acceptance tests below.

### Static Analysis

Two static analysis tools were used throughout development to enforce consistency and catch issues before runtime:

- **Clang-Tidy** — a compile-time analyser used to increase the scrutiny of compiler warnings, integrated directly into the development workflow.
- **ReSharper** — configured against a custom set of coding conventions to provide real-time warnings, errors, and recommendations while writing C++ and C# code.

Both tools were configured to align with the coding conventions adopted across the project (consistent naming, no global variables, explicit member access, controlled use of dynamic memory), reinforcing the MISRA/DO-178C-inspired philosophy of writing predictable, maintainable, and low-risk code.

### Acceptance Tests

A suite of top-down acceptance tests was developed to validate the system from a user's perspective, covering application lifecycle (open/close/minimise/reopen), configuration and deployment of NEON, and live control and monitoring of the simulated vehicle (acceleration, braking, gear changes, warnings, refuelling, and graceful undeployment). These tests were modelled on acceptance testing practices encountered in industry, where tests are segmented by feature area so that only the relevant subset needs to be re-run after a given change, and are executed before any significant changes are merged.

All defined acceptance tests passed, demonstrating that the end-to-end system behaves correctly when driven through the VSP.

### Simulation Analysis

Beyond functional correctness, the physical plausibility of the simulation was evaluated under a range of driving scenarios:

- **Gear changes** — verifying expected RPM behaviour on upshift/downshift, including correct triggering of the stall condition above 110% of redline.
- **Acceleration** — validating the Gaussian torque curve, fuel consumption (including rich-mixture behaviour near redline), and thermal rise under sustained load, including hysteresis-based overheating detection.
- **Braking and engine braking** — confirming coasting deceleration from rolling resistance and drag, proportional braking response, and correct RPM behaviour as speed approaches zero.

This analysis confirmed that the engine, fuel, and thermal models produce realistic, internally consistent behaviour across a variety of scenarios, even though they are based on theoretical approximations rather than empirical hardware data.



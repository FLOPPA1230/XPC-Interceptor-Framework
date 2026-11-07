# XPC-Nexus

## Overview
XPC-Nexus is an advanced security research framework designed for analyzing, intercepting, and fuzzing XPC inter-process communication on iOS. It is built for security researchers to evaluate the robustness of system daemons and identify vulnerabilities in input validation at Ring 3.

## Features
- XPC Service Enumeration: Discover active daemons and their Mach services.
- Real-time Interception: Hook and log XPC messages between processes.
- Intelligent Fuzzing: Generate randomized and mutated payloads to test service stability.
- AES-Encrypted Logging: Securely log intercepted traffic to prevent data leakage.

## Architecture
The framework is written in pure C and consists of the following modules:
- daemon_utils: Reads LaunchDaemons plists to map XPC services.
- interceptor: Uses Dobby or fishhook to hook XPC send/receive functions.
- fuzzer: Multi-threaded engine for generating and dispatching fuzzed payloads.
- logger: Thread-safe, rotating, and encrypted logging system.

## Requirements
- A Jailbroken iOS device running iOS 14.0 to iOS 16.7.12.
- For iOS 16.6 to 18.7.1 or iOS 26.0, the device must use A12/A13 chips and be jailbroken with Dopamine 3.
- Root privileges to access process memory and system daemons.
- Clang compiler and Xcode Command Line Tools.

## Compilation
Use the provided Makefile to compile the framework:
    make clean
    make

## Installation
Transfer the compiled binary to your jailbroken device:
    scp xpc_nexus root@IPHONE_IP:/usr/bin/
    ssh root@IPHONE_IP chmod +x /usr/bin/xpc_nexus

## Usage
Run the tool with root privileges. Example commands:
    xpc_nexus --enum
    xpc_nexus --intercept com.apple.backboardd
    xpc_nexus --fuzz com.apple.SpringBoard

## Security Warning
This tool is intended for authorized security research and vulnerability assessment on devices you own or have explicit permission to test. The authors assume no liability for any misuse or damage caused by this framework.

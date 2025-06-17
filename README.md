# Tello C++ SDK

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)]()
[![Header-Only](https://img.shields.io/badge/Header--Only-blueviolet.svg)](https://en.wikipedia.org/wiki/Header-only)

A modern, single-header C++20 library for controlling the Ryze Tello drone. This library provides a clean, intuitive, and robust interface to the official Tello SDK, making it easy to integrate drone control into your C++ applications.

It is designed to be self-contained and easy to use, with an embedded lightweight, cross-platform UDP socket implementation.

---

## Key Features

-   **✅ Full Tello SDK 2.0 Support**: Implements all control, set, and read commands from the official SDK.
-   **📦 Header-Only**: Simply include `tello.h` in your project with no separate compilation steps required.
-   **🖥️ Cross-Platform**: Fully functional on both Windows and Linux environments.
-   **🚀 Modern C++20**: Leverages features like `<format>`, `<concepts>`, `<jthread>`, and `<ranges>` for a clean and efficient API.
-   **📡 Asynchronous State Updates**: Receives drone telemetry (attitude, battery, height, etc.) on a dedicated background thread without blocking your main logic.
-   **🎯 Mission Pad Support**: Provides a simple and explicit API for Mission Pad detection and navigation.
-   **💡 Simple Logging**: Includes built-in colored logging for easy debugging, which can be enabled by defining `TELLO_DEBUG`.
-   **🔁 Robust Connection**: Automatically retries the initial connection command to ensure a stable start.

## Requirements

-   A C++20 compliant compiler (e.g., MSVC v19.29+, GCC 10+, Clang 12+).
-   A Wi-Fi connection to the Tello drone's network.

## Installation

As a header-only library, installation is straightforward:

1.  Download the latest version of [`tello.h`](tello.h).
2.  Place the file in your project's include directory.
3.  Include it in your source file: `#include "tello.h"`.

> [!NOTE]
> Before running your application, ensure your computer is connected to the Tello drone's Wi-Fi network (e.g., "TELLO-XXXXX").

## Quick Start Example

The following example demonstrates how to connect to the drone, perform a simple flight pattern, check the battery, and land safely.

```cpp
#include <iostream>
#include <string>
#include "tello.h"

int main() {
    Tello tello;

    // Connect to the drone's network
    if (!tello.connect()) {
        std::cerr << "Failed to connect to Tello." << std::endl;
        return 1;
    }

    // Set a timeout for flight actions (in milliseconds)
    // Otherwise, commands can block indefinitely if the action is not completed.
    tello.set_action_timeout(15000); // 15 seconds

    // Takeoff
    if (!tello.takeoff()) {
        std::cerr << "Tello failed to take off." << std::endl;
        return 1;
    }

    // Check battery level
    float battery = tello.get_battery_level();
    std::cout << "Battery level: " << battery << "%" << std::endl;

    // Fly in a square pattern
    std::cout << "Flying a square pattern..." << std::endl;
    for (int i = 0; i < 4; ++i) {
        tello.move_forward(50);
        tello.turn_right(90);
    }

    // Land
    std::cout << "Mission complete. Landing." << std::endl;
    tello.land();

    return 0;
}

# Tello C++ Library

A single-header C++20 library for controlling the Ryze Tello drone. It provides a simple, modern C++ interface to the Tello SDK 2.0.

This library is header-only, requiring no separate compilation steps. It includes a lightweight, cross-platform UDP socket implementation.

## Features

-   **Full Tello SDK 2.0 Support**: Implements all control, set, and read commands.
-   **Header-Only**: Simply include `tello.h` in your project.
-   **Cross-Platform**: Works on both Windows and Linux.
-   **Modern C++20**: Utilizes modern C++ features like `<format>`, `<concepts>`, and `<jthread>` for a clean and efficient API.
-   **Asynchronous State Updates**: Receives drone state information (attitude, battery, etc.) on a background thread.
-   **Mission Pad Support**: Includes helpers for Mission Pad detection and navigation.
-   **Simple Logging**: Built-in colored logging for easy debugging.

## Requirements

-   A C++20 compliant compiler (e.g., GCC 10+, Clang 12+, MSVC v19.29+).
-   A network connection to the Tello drone.

## How to Use

1.  Download `tello.h`.
2.  Include it in your C++ source file.
3.  Instantiate the `Tello` class.
4.  Connect to the drone and start sending commands.

You must be connected to the Tello's Wi-Fi network before running your program.

## Quick Start Example

The following example demonstrates how to connect to the drone, take off, fly in a square, and land.

```cpp
#include <iostream>
#include "tello.h"

int main() {
    Tello tello;

    // Connect to the drone
    if (!tello.connect()) {
        std::cerr << "Failed to connect to Tello." << std::endl;
        return 1;
    }

    // Set a timeout for actions, otherwise they block forever
    tello.set_action_timeout(10000); // 10 seconds

    // Takeoff
    if (!tello.takeoff()) {
        std::cerr << "Tello failed to take off." << std::endl;
        return 1;
    }

    std::cout << "Flying a square pattern..." << std::endl;

    // Fly in a square
    tello.move_forward(50);
    tello.turn_right(90);
    tello.move_forward(50);
    tello.turn_right(90);
    tello.move_forward(50);
    tello.turn_right(90);
    tello.move_forward(50);
    tello.turn_right(90);

    // Land
    tello.land();

    std::cout << "Mission complete. Landing." << std::endl;

    return 0;
}

```
## Credits
[HerrNamenlos123](https://github.com/HerrNamenlos123/tello) for original library# Tello C++ Library

A single-header C++20 library for controlling the Ryze Tello drone. It provides a simple, modern C++ interface to the Tello SDK 2.0.

This library is header-only, requiring no separate compilation steps. It includes a lightweight, cross-platform UDP socket implementation.

## Features

-   **Full Tello SDK 2.0 Support**: Implements all control, set, and read commands.
-   **Header-Only**: Simply include `tello.h` in your project.
-   **Cross-Platform**: Works on both Windows and Linux.
-   **Modern C++20**: Utilizes modern C++ features like `<format>`, `<concepts>`, and `<jthread>` for a clean and efficient API.
-   **Asynchronous State Updates**: Receives drone state information (attitude, battery, etc.) on a background thread.
-   **Mission Pad Support**: Includes helpers for Mission Pad detection and navigation.
-   **Simple Logging**: Built-in colored logging for easy debugging.

## Requirements

-   A C++20 compliant compiler (e.g., GCC 10+, Clang 12+, MSVC v19.29+).
-   A network connection to the Tello drone.

## How to Use

1.  Download `tello.h`.
2.  Include it in your C++ source file.
3.  Instantiate the `Tello` class.
4.  Connect to the drone and start sending commands.

You must be connected to the Tello's Wi-Fi network before running your program.

## Quick Start Example

The following example demonstrates how to connect to the drone, take off, fly in a square, and land.

```cpp
#include <iostream>
#include "tello.h"

int main() {
    Tello tello;

    // Connect to the drone
    if (!tello.connect()) {
        std::cerr << "Failed to connect to Tello." << std::endl;
        return 1;
    }

    // Set a timeout for actions, otherwise they block forever
    tello.set_action_timeout(10000); // 10 seconds

    // Takeoff
    if (!tello.takeoff()) {
        std::cerr << "Tello failed to take off." << std::endl;
        return 1;
    }

    std::cout << "Flying a square pattern..." << std::endl;

    // Fly in a square
    tello.move_forward(50);
    tello.turn_right(90);
    tello.move_forward(50);
    tello.turn_right(90);
    tello.move_forward(50);
    tello.turn_right(90);
    tello.move_forward(50);
    tello.turn_right(90);

    // Land
    tello.land();

    std::cout << "Mission complete. Landing." << std::endl;

    return 0;
}

```
## Credits
[HerrNamenlos123](https://github.com/HerrNamenlos123/tello) for original library
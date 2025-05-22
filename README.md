# OpenFX Modern Template

This repository serves as a starting point for creating OpenFX effects using modern tools and practices. It includes a sample effect, `invert.cpp`, which has been adapted from the [official OpenFX repository](https://github.com/AcademySoftwareFoundation/openfx) and updated to use modern C++ standards.

## Tools and Dependencies

The project is built using the following tools:

- **CMake**: A cross-platform build system generator.
- **vcpkg**: A C++ library manager that simplifies the installation of dependencies.
- **Ninja**: A small build system with a focus on speed.
- **clang-format**: A tool to format C++ code according to style guidelines.
- **clang-tidy**: A clang-based C++ linter tool.

## Prerequisites

Before you can build and run this project, you need to have the following tools installed on your system:

1. **CMake**: Available via most package managers (e.g., `apt`, `winget`).
2. **vcpkg**: Available via most package managers (e.g., `apt`, `winget`).
3. **Ninja**: Available via most package managers (e.g., `apt`, `winget`).
4. **clang-format** and **clang-tidy**: These are typically included with the LLVM toolchain and are often integrated into modern code editors.

Additionally, you need to set the `VCPKG_ROOT` environment variable to point to your vcpkg installation directory.

## Setting Up the Project

1. Clone the repository:
    ```sh
    git clone https://github.com/Hashory/openfx-modern-template.git
    cd openfx-modern-template
    ```

2. Create a build directory and configure the project using CMake:
    ```sh
    cmake --preset=vcpkg
    ```

3. Build the project:
    ```sh
    cmake --build build
    ```

    After the build is complete, the output will be located at `invert.ofx.bundle/Contents/Your-OS/invert.ofx`.

## Project Structure

- `invert.cpp`: The main source file for the sample OpenFX effect. This file has been adapted from the official OpenFX repository and updated to use modern C++ standards.
- `CMakeLists.txt`: The CMake configuration file for the project.
- `.clang-format`: Configuration file for clang-format.
- `.clang-tidy`: Configuration file for clang-tidy.
- `vcpkg.json`: Configuration file for vcpkg dependencies.

## License

This project is licensed under the BSD-3-Clause License, the same as the [official OpenFX repository](https://github.com/AcademySoftwareFoundation/openfx). See the [LICENSE.md](LICENSE.md) file for details.

## Acknowledgements

The `invert.cpp` file is based on code from the [official OpenFX repository](https://github.com/AcademySoftwareFoundation/openfx) and has been updated to use modern C++ standards.

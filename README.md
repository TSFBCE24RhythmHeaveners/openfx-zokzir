# Zokzir OpenFX Alpha

This repository includes two basic OpenFX plugins: Saturation and Droste.

## Tools and Dependencies

The project is built using the following tools:

- **CMake**: A cross-platform build system generator.
- **VcPkg**: A C++ library manager that simplifies the installation of dependencies.
- **Ninja**: A small build system with a focus on speed.
- **clang-format**: A tool to format C++ code according to style guidelines.
- **clang-tidy**: A clang-based C++ linter tool.

## Prerequisites

Before you can build and run this project, you need to have the following tools installed on your system:

1. **CMake**: Available via most package managers (e.g., `apt`, `winget`).
2. **VcPkg**: Available via most package managers (e.g., `apt`, `winget`).
3. **Ninja**: Available via most package managers (e.g., `apt`, `winget`).
4. **clang-format** and **clang-tidy**: These are typically included with the LLVM toolchain and are often integrated into modern code editors.

Additionally, you need to set the `VCPKG_ROOT` environment variable to point to your vcpkg installation directory.

## Setting Up the Project

1. Clone the repository:
    ```sh
    git clone https://github.com/TSFBCE24RhythmHeaveners/openfx-zokzir.git
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

    After the build is complete, the output will be located at `ZokzirEFFECT.ofx.bundle/Contents/Your-OS/ZokzirEFFECT.ofx`.

## Project Structure

- `zokzireffect.cpp`: Zokzir effect base
- `CMakeLists.txt`: The CMake configuration file for the project.
- `.clang-format`: Configuration file for clang-format.
- `.clang-tidy`: Configuration file for clang-tidy.
- `vcpkg.json`: Configuration file for vcpkg dependencies.

## License

This project is licensed under the BSD-3-Clause License.

## Acknowledgements

Alex Zuma 2025 posted that Bruor Macanelic words can also be used in naming things. https://youtube.com/post/Ugkx6mWr4lfwGGR9ViHph_JgPFVCPqtqh1Zy

# 3D Sorting Algorithm Visualizer

An interactive 3D Sorting Algorithm Visualizer built in C++ using **Raylib 6.0**. This project visualizes various sorting algorithms in full 3D space with smooth animations, dynamic color gradients, arc-based swapping trajectories, particle effects, and audio feedback.

---

## Key Features

* **3D Visualizations & Animations**: Bars represent array elements with height and color gradient; swaps feature 3D arc jumps and particle burst effects.
* **Multiple Sorting Algorithms**:
  * **Bubble Sort**
  * **Selection Sort**
  * **Insertion Sort**
  * **Quick Sort**
  * **Merge Sort**
  * **Heap Sort**
* **Array Initializations**:
  * Random
  * Reversed
  * Nearly Sorted
  * Few Unique
* **Interactive Controls & Camera**: Orbit, pan, and zoom 3D camera controls with real-time UI controls for algorithm selection, animation speed, array size, step-by-step execution, sound, and visual theme options.

---

## Repository Structure

```text
3D-Sort-Animation-/
+-- .vscode/               # VS Code workspace settings, build tasks, and launch configurations
|   +-- c_cpp_properties.json
|   +-- launch.json
|   \-- tasks.json
+-- .gitignore             # Ignored build artifacts, binaries, and local dependencies
+-- CMakeLists.txt         # CMake build configuration
+-- Makefile               # Make build configuration for MinGW / GCC
+-- build_and_run.bat      # Windows automated build script
+-- main.cpp               # Main 3D Sorting Visualizer source code
\-- README.md              # Project documentation
```

---

## Requirements & Setup

### Prerequisites
* **C++ Compiler**: GCC (MinGW-w64) supporting C++17 or MSVC.
* **Raylib 6.0**: Precompiled binaries or header/library files installed.

---

## Building & Running

### Option 1: Using `build_and_run.bat` (Windows MinGW)
If Raylib is installed at `C:\raylib-6.0_win64_mingw-w64\raylib-6.0_win64_mingw-w64`:
```cmd
build_and_run.bat
```

### Option 2: Using `g++` directly
```cmd
g++ main.cpp -O2 -std=c++17 -IC:/raylib-6.0_win64_mingw-w64/raylib-6.0_win64_mingw-w64/include -LC:/raylib-6.0_win64_mingw-w64/raylib-6.0_win64_mingw-w64/lib -lraylib -lopengl32 -lgdi32 -lwinmm -o sorting_visualizer.exe
./sorting_visualizer.exe
```

### Option 3: Using `Make`
```bash
make
./sorting_visualizer.exe
```

### Option 4: Using `CMake`
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

---

## License
This project is open-source under the MIT License.

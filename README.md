# Evaluate raylib

Evaluating [raylib](http://www.raylib.com/) with some simple test programs.
> ## **raylib** is not a prerequisite
>
> It may seem strange, but you don't need to install **raylib** as the build steps will download it automatically.
> See `CMakeLists.txt` for details, or for an alternative if you encounter a problem.

## Getting Started

Pick the platform that you want to build for from one of these links.

- [Building and running on Windows](#building-and-running-on-windows)
- [Building for the web with Emscripten](#building-for-the-web-with-emscripten)
- [Building for Raspberry Pi desktop](#building-for-raspberry-pi-desktop)
- [Building for Raspberry Pi 4 / 400 native mode](#building-for-raspberry-pi-4--400-native-mode)

# Building and running on Windows

## Prerequisites

### **CMake** and **Ninja**

You'll need [**CMake**](https://cmake.org/) and (optionally) [**Ninja**](https://ninja-build.org/). You can install
these with a package manager, such as [**scoop**](https://scoop.sh/) or [**chocolatey**](https://chocolatey.org/).

#### Installing **CMake** and **Ninja** using **scoop**

```
C:> scoop install cmake
C:> scoop install ninja
```
#### Installing **CMake** and **Ninja** using **chocolatey**

```
C:> choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=User'
C:> choco install ninja
```
### **Git**

You'll need [**git**](https://git-scm.com/) to clone the repo. You can install this [directly](https://git-scm.com/), or by using [**scoop**](https://scoop.sh/) or [**chocolatey**](https://chocolatey.org/).

#### Installing **git** using **scoop**

```
C:> scoop install git
```
#### Installing **git** using **chocolatey**

```
C:> choco install git
```
### C and C++ compilers

You'll also need compilers for **C** and **C++**. These instructions assume that you
have [Visual Studio with the C++ workload](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-160)
or the [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019). However, feel free to use something else such as **clang** or **gcc** and **g++**.

## Building

Open a [Visual Studio developer command prompt window](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160#developer_command_prompt). The `PATH` in this window includes the Visual Studio command line tools, which means that **CMake** will be able to detect them.

Clone the repo with **git** then change directory into it.

```
C:> git clone https://github.com/badlydrawnrod/evaluate-raylib
C:> cd evaluate-raylib
```

Create a build directory and go into it.

```
C:> mkdir build-debug
C:> cd build-debug
```

Generate a build system with **CMake**. This will detect your C and C++ compilers, then it will download **raylib** itself using **CMake**'s `Fetch_Content` module.

```
C:> cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -S ..
```

From the same directory, tell **CMake** to call the build system (**Ninja**) to compile and link the project. This will
build everything, includng **raylib**. It will finish in seconds, as **Ninja** is lightning quick.

```
C:> cmake --build .
```

## Running

From the build directory, go into the `simple/` directory and run `track3d.exe`.
```
C:> cd simple
C:> track3d.exe
```

You should see a retro-looking 3d racetrack.

# Building for the web with Emscripten

## Prerequisites

### **CMake** and **Ninja**

You'll need [**CMake**](https://cmake.org/) and (optionally) [**Ninja**](https://ninja-build.org/). You can install
these with a package manager, such as [**scoop**](https://scoop.sh/) or [**chocolatey**](https://chocolatey.org/).

#### Installing **CMake** and **Ninja** using **scoop**

```
C:> scoop install cmake
C:> scoop install ninja
```
#### Installing **CMake** and **Ninja** using **chocolatey**

```
C:> choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=User'
C:> choco install ninja
```
### **Git**

You'll need [**git**](https://git-scm.com/) to clone the repo. You can install this [directly](https://git-scm.com/), or by using [**scoop**](https://scoop.sh/) or [**chocolatey**](https://chocolatey.org/).

#### Installing **git** using **scoop**

```
C:> scoop install git
```
#### Installing **git** using **chocolatey**

```
C:> choco install git
```
### **Emscripten**

To compile for the web, you'll need to install [**Emscripten**](https://emscripten.org/), which is a compiler toolchain that lets you compile to [**WebAssembly**](https://developer.mozilla.org/en-US/docs/WebAssembly).

Install **Emscripten** using [these instructions](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions).
## Building

Open a command prompt.

Clone the repo with **git** then change directory into it.

```
C:> git clone https://github.com/badlydrawnrod/evaluate-raylib
C:> cd evaluate-raylib
```

Create a build directory and go into it.

```
C:> mkdir build-emscripten-debug
C:> cd build-emscripten-debug
```

Generate a build system with **CMake**. This will detect the **Emscripten** toolchain, then it will download **raylib** itself using **CMake**'s `Fetch_Content` module.

Change `EMSCRIPTEN_ROOT_PATH` and `CMAKE_TOOLCHAIN_FILE` to match where **Emscripten** is installed on your system. In the example below, it is installed in `D:\Tools\emsdk`.

```
C:> cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -S ..
    -DNO_MSAA=ON
    -DPLATFORM=Web
    -DEMSCRIPTEN_ROOT_PATH=D:\Tools\emsdk\upstream\emscripten
    -DCMAKE_TOOLCHAIN_FILE=D:\Tools\emsdk\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake 
```

From the same directory, tell **CMake** to call the build system (**Ninja**) to compile and link the project. This will build everything, includng **raylib**. It will finish in seconds, as **Ninja** is lightning quick.

```
C:> cmake --build .
```
## Running

Run a web server from the build directory. You can use a recent version of [**python**](https://www.python.org/) for this.
```
C:> python -m http.server 8080
```

Open a web browser and navigate to http://localhost:8080/simple/track3d.html. You should see a retro-looking 3d racetrack.

# Building for Raspberry Pi desktop

Note that these instructions are known to work for a physical Raspberry Pi (tested on a Raspberry Pi 400) and for a VM running [**Raspberry Pi Desktop**](https://www.raspberrypi.org/software/raspberry-pi-desktop/).

## Prerequisites

### **CMake** and **Ninja**

You'll need [**CMake**](https://cmake.org/) and (optionally) [**Ninja**](https://ninja-build.org/). Install them with `apt install`.

#### Installing **CMake** and **Ninja**

```
$ sudo apt install cmake
$ sudo apt install ninja-build
```

### **Git**

You'll need [**git**](https://git-scm.com/) to clone the repo. Install it with `apt install`.

#### Installing **git**

```
$ sudo apt install git
```

### Packages for desktop development

To compile for Raspberry Pi desktop and run in a window, you'll need the desktop development libraries. These are listed in [compiling raylib source code](https://github.com/raysan5/raylib/wiki/Working-on-Raspberry-Pi#compiling-raylib-source-code). Install them with `apt install`.

#### Installing packages for desktop development
```
$ sudo apt install --no-install-recommends raspberrypi-ui-mods lxterminal gvfs
$ sudo apt install libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev
  libxi-dev libasound2-dev mesa-common-dev libgl1-mesa-dev
```
## Building

Open a terminal window.

Clone the repo with **git** then change directory into it.

```
$ git clone https://github.com/badlydrawnrod/evaluate-raylib
$ cd evaluate-raylib
```

Create a build directory and go into it.

```
$ mkdir build-debug
$ cd build-debug
```

Generate a build system with **CMake**. This will detect the compiler, then it will download **raylib** itself using **CMake**'s `Fetch_Content` module.

```
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -S ..
  -DNO_MSAA=ON
  -DPLATFORM=Desktop -DGRAPHICS=GRAPHICS_API_OPENGL_21
```

From the same directory, tell **CMake** to call the build system (**Ninja**) to compile and link the project. This will build everything, includng **raylib**. It will finish in seconds, even on a Raspberry Pi, as **Ninja** is lightning quick.

```
C:> cmake --build .
```
## Running

From the build directory, go into the `simple/` directory and run `track3d`.
```
$ cd simple
$ ./track3d
```

You should see a retro-looking 3d racetrack.

# Building for Raspberry Pi 4 / 400 native mode
## Prerequisites

### **CMake** and **Ninja**

You'll need [**CMake**](https://cmake.org/) and (optionally) [**Ninja**](https://ninja-build.org/). Install them with `apt install`.

#### Installing **CMake** and **Ninja**

```
$ sudo apt install cmake
$ sudo apt install ninja-build
```

### **Git**

You'll need [**git**](https://git-scm.com/) to clone the repo. Install it with `apt install`.

#### Installing **git**

```
$ sudo apt install git
```
### Packages for native mode development

To compile for Raspberry Pi 4 / 4000 native mode, you'll need the [**DRM**](https://en.wikipedia.org/wiki/Direct_Rendering_Manager) libraries. These are listed in [compiling raylib source code](https://github.com/raysan5/raylib/wiki/Working-on-Raspberry-Pi#compiling-raylib-source-code). Install them with `apt install`.

#### Installing packages for native mode development
```
$ sudo apt install libdrm-dev libegl1-mesa-dev libgles2-mesa-dev libgbm-dev
```
## Building

Open a terminal window.

Clone the repo with **git** then change directory into it.

```
$ git clone https://github.com/badlydrawnrod/evaluate-raylib
$ cd evaluate-raylib
```

Create a build directory and go into it.

```
$ mkdir build-drm-debug
$ cd build-drm-debug
```

Generate a build system with **CMake**. This will detect the compiler, then it will download **raylib** itself using **CMake**'s `Fetch_Content` module.

```
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -S ..
  -DNO_MSAA=ON
  -DPLATFORM=DRM
```

From the same directory, tell **CMake** to call the build system (**Ninja**) to compile and link the project. This will build everything, includng **raylib**. It will finish in seconds, even on a Raspberry Pi, as **Ninja** is lightning quick.

```
C:> cmake --build .
```
## Running

Open a local tty, e.g., by pressing `Ctrl`+`Alt`+`F2`, and log in.

Change directory to the build directory from the previous step.
```
$ cd evaluate-raylib/build-drm-debug
```

From the build directory, go into the `simple/` directory and run `track3d`.
```
$ cd simple
$ ./track3d
```

You should see a retro-looking 3d racetrack.

> ### Note
> At the time of writing, I have only tried native mode once. The native mode builds have a couple of known issues:
> 1. They don't scale to the screen, so they'll appear squashed unless you happen to be running them on a 1280x720 screen.
> 1. The keyboard handling doesn't seem to work. This is probably something to do with how I'm checking for keypresses / releases.

# A minimal application with Vulkan compute shaders

In this repository you will find a minimal Vulkan application that uses compute shaders.
You can use this as a starting point for your project.

I created [a video](https://www.youtube.com/watch?v=KN9nHo9kvZs) explaining Vulkan compute shaders and this code, alongside the corresponding [blog post](https://necrashter.github.io/ceng469/project/compute-shaders-intro) which contains more information about the references.
You can also access the slides from there.

The Vulkan code is based on [this tutorial](https://bakedbits.dev/posts/vulkan-compute-example/).
However, unlike the tutorial, the shader in this repository is written in GLSL instead of HLSL.
I also created a build system with CMake.


## Building

CMake is used for building the application.
Vulkan SDK needs to be installed.
GLSL compiler `glslc` must be installed and in your `PATH`.

Create build folder:
```sh
$ mkdir bin
$ cd bin
```

Configure:
```sh
$ cmake ..
```

Compile & run:
```sh
$ make
$ ./main
```

I compiled and tested this program only on Linux.

Contributions are welcome.


## Expected Output

The program just squares the numbers 0 to 9 in a compute shader.

```
Device Name    : Intel(R) HD Graphics 630 (KBL GT2)
Vulkan Version : 1.2.182
Compute Queue Family Index: 0
Memory Type Index: 0
Memory Heap Size : 11 GB

INPUT:  0 1 2 3 4 5 6 7 8 9 
OUTPUT: 0 1 4 9 16 25 36 49 64 81
```

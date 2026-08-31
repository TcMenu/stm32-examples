# STM32Cube embedded menu UI demos using tcMenu framework

In this repository we host a few STM32 starter examples for tcMenu. If you're on the same board (or very similar) as us, you've got an absolute starter that can be used immediately. Otherwise, a quick trip through STM32CubeMX should result in a working project.

## Setting up

We use CLion with STM32CubeMX and standard build tools, but any IDE compatible with STM32Cube should work just fine. There are two projects and each has an example that showcases the hardware on a particular board.

At the top level of each project, you need to create a `tc-libs` folder and either clone or symlink the required libraries there.

Here is what I use on macOS/Linux to link all the libraries in, a similar script could be used on powershell for windows:

```
mkdir tc-libs
cd tc-libs
export TCC_LIB_DIR=<your home dir>
ln -s $TCC_LIB_DIR/TcMenuLog
ln -s $TCC_LIB_DIR/TaskManagerIO
ln -s $TCC_LIB_DIR/tcUnicodeHelper
ln -s $TCC_LIB_DIR/IoAbstraction
ln -s $TCC_LIB_DIR/tcMenu
ln -s $TCC_LIB_DIR/Adafruit-GFX-mbed-fork
```

For example, if we are in an STM32Cube project called `ProjectA`, the structure would be:

```
	ProjectA/
		tc-libs/
			tcMenu
			tcUnicodeHelper
			IoAbstraction
			TcMenuLog
			TaskManagerIO
			Adafruit-GFX-mbed-fork
```

We normally symlink the library directories into the `tc-libs` folder, but you could clone them there as well.

In each top-level project CMake file, you'll see we've added all those subdirectories to the build, and then added them as libraries too.

```cmake
add_subdirectory(cmake/stm32cubemx)
add_subdirectory(tc-libs/TcMenuLog/cmake)
add_subdirectory(tc-libs/TaskManagerIO/cmake)
add_subdirectory(tc-libs/tcUnicodeHelper/cmake)
add_subdirectory(tc-libs/IoAbstraction/cmake)
add_subdirectory(tc-libs/tcMenu/cmake)
add_subdirectory(tc-libs/Adafruit-GFX-mbed-fork/cmake)

# ... Other code ...

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
        TcMenuLog TaskManagerIO tcUnicodeHelper IoAbstraction AdafruitGFXNativePort tcMenu
)
```

## Licenses

The code written by the authors of tcMenu is under the Apache 2.0 license. This makes it safe to copy and use in your own designs. However, this repository also contains STM32Cube-provided code, which is subject to STM's own license terms (typically BSD-3-Clause). If a specific folder contains its own license file, those terms apply to the code within that folder.

For full details, please refer to the [LICENSE](LICENSE) file in the root of this repository.


## Example 1: STM32F429 DISC1

This project is called `TestLTDC`. It uses our frame-buffer support classes from the Adafruit fork project that we maintain. It has excellent display performance and touch support. It is one of our showcase examples.

## Example 2: STM32F439 Nucleo

This project is called `TestOledEth` and provides support for an OLED screen, again using our Adafruit fork project. It uses SPI and performance is excellent.


# STM32Cube embedded menu UI demos using tcMenu framework

In this repository we host a few STM32Cube starter/example projects for tcMenu. They target the STM32F4 range of devices, and if you're on the same development board as the example it should work out of the box. Otherwise, I've tried to document how to start out in STMCubeMX.


## Example 1: STM32F429 DISC1

This project is called `TestLTDC`. It uses our frame-buffer support classes from the Adafruit fork project that we maintain. It has excellent display performance and touch support. It is one of our showcase examples. FreeRTOS is enabled. Logging is configured to the ST-Link adapter.

## Example 2: STM32F439 Nucleo144

This project is called `TestOledEth` and provides support for an OLED screen with rotary encoder, again using our Adafruit fork project. The display uses SPI and performance is excellent. FreeRTOS in enabled. Logging is configured to the ST-Link adapter.

## What is tcMenu?

TcMenu is a framework with a [web based embedded menu designer](https://designer.thecoderscorner.com/) to build your initial UI/menu structure. Generally speaking the output code will run right away on any working STMCube setup following this page. It rapidly generates a moderately capable UI with exceptionally low footprint.

* Structured menus (settings, calibration, control panels).
* Fast onboarding with web based menu designer.
* No allocation outside of setup, heap is exceptionally stable.
* OLED, LTDC, SPI-TFT, LCD devices needing a simple grid based UI.
* TaskManagerIO allows for simple scheduling and is RTOS safe.
* Products where predictability matters more than animation.

## Required software

You'll need STM32CubeMX UI, ARM build tools, and an IDE. We use CLion with STM32CubeMX and the cube provided build tools, but any IDE compatible with STM32Cube should work just fine. Resources:

* STMCube - https://www.st.com/en/development-tools/stm32-software-development-tools.html
* CLion (or other IDE) https://www.jetbrains.com/clion/
* TcMenu libraries - see https://github.com/TcMenu/tcLibraryDev

## Getting started (loading one of the examples)

A full walk through of STMCubeMX is beyond this page and is well documented elsewhere. The following instructions assume you've output a project and loaded it into your IDE.

### Adding the tcMenu framework libraries.

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

### Updating the CMakeLists.txt files

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

Then add the following build property for cmake to know the platform `-DBUILD_FOR_STM32CUBE=true`.

## Where's the tcMenu source?

I tend to create another source folder in the project root called `CoreMenu` and put all my source in that folder, keep it separate from other sources. Whenever I tend to run through STMCubeMX I first commit everything that I have to git, so that I have a clean starting point in terms of differences. Then I can quickly restore any changes lost during the round trip.

### A few methods need implementing

```
// milliseconds since start, see our examples
uint32_t millis(void);
// microseconds since start, see our examples
uint32_t micros(void);
// a yield function - see the examples
void yield(void);

// you need to externally declare any spi that's used with SPIHelper. In main.h
extern SPI_HandleTypeDef hspi3;

// in your app main, declare a UART to output logging to, or turn off logging
extern UART_HandleTypeDef huart3;
UART_HandleTypeDef* loggingUart = &huart3;
```

## IoAbstraction differences

Unlike other IoAbstraction implementations, this one assumes that you've mapped the pins manually in CubeMX and then you map each configured GPIO to a pin

```
// map GPIO PF12 to pin 1 in IoAbstraction.
appendIoaPin(StmGpioDesc(GPIOF, GPIO_PIN_12, 1));
```

The pin mappings are stored in an array starting at `0` up to `STM32_IOA_GPIO_ARR_SIZE` (default 16). The array mapping is an unchanged non-contended array in memory so is interrupt safe. It is assumed that all such mappings occur at startup only. If you need more than 16 mappings, define the above variable as a build flag.

## Licenses

The code written by the authors of tcMenu is under the Apache 2.0 license. This makes it safe to copy and use in your own designs. However, this repository also contains STM32Cube-provided code, which is subject to STM's own license terms (typically BSD-3-Clause). If a specific folder contains its own license file, those terms apply to the code within that folder.

For full details, please refer to the [LICENSE](LICENSE) file in the root of this repository.

# Copyright 2019 Tinic Uro
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

set(PRJ_NAME Pendant2019)

cmake_minimum_required(VERSION 3.10)

option(EMULATOR_BUILD "Emulator Build" OFF)
option(MCP_BUILD "MCP Build" OFF)

project(${PRJ_NAME} C CXX ASM)

if(NOT CMAKE_BUILD_TYPE) 
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if (NOT EMULATOR_BUILD)
	set(MCU_MODEL SAMD51G18A)
	set(MCU_ARCH cortex-m4)
	set(MCU_FPU fpv4-sp-d16)
	set(MCU_FLOAT_ABI softfp)
	set(MCU_LINKER_SCRIPT Device_Startup/samd51g18a_flash.ld)

	set(CMAKE_SYSTEM_NAME Generic)
	set(CMAKE_SYSTEM_VERSION 1)

	set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
	set(CMAKE_C_COMPILER arm-none-eabi-gcc)

	set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
	set(CMAKE_SIZE arm-none-eabi-size)
	add_definitions(-D__${MCU_MODEL}__)

	set(COMMON_FLAGS "-Werror -Wall -Wextra -Wno-strict-aliasing -mcpu=${MCU_ARCH} -ffunction-sections -ffast-math -mthumb -mfloat-abi=${MCU_FLOAT_ABI} -mlong-calls")

	if(CMAKE_C_COMPILER STREQUAL clang)
		set(COMMON_FLAGS "${COMMON_FLAGS} -target arm-none-eabi")
	endif()

	if(MCU_FLOAT_ABI STREQUAL hard)
		set(COMMON_FLAGS "${COMMON_FLAGS} -mfpu=${MCU_FPU}")
	endif()

	set(CMAKE_EXE_LINKER_FLAGS "--specs=nano.specs -Wl,--warn-unresolved-symbols -Wl,--start-group -lm -Wl,--end-group -Wl,-gc-sections -T${CMAKE_SOURCE_DIR}/${MCU_LINKER_SCRIPT}")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")

	set(CMAKE_OSX_SYSROOT "")
	set(CMAKE_C_LINK_FLAGS "")
	set(CMAKE_CXX_LINK_FLAGS "")
	set(CMAKE_LINKER_FLAGS "")

	file(GLOB_RECURSE HAL_SRC hal/*.c)
	file(GLOB_RECURSE HPL_SRC hpl/*.c)
	file(GLOB_RECURSE USB_SRC usb/*.c)
	file(GLOB_RECURSE DEVICE_STARTUP Device_Startup/*.c)

	set(SOURCE_FILES ${HAL_SRC} ${HPL_SRC} ${USB_SRC} ${DEVICE_STARTUP})

	if (MCP_BUILD)
		add_definitions(-DMCP)
	endif (MCP_BUILD)
	

	include_directories(Config)
	include_directories(hal/utils/include)
	include_directories(hal/include)
	include_directories(hal/include)
	include_directories(hri)
	include_directories(hpl)
	include_directories(hpl/pm)
	include_directories(hpl/gclk)
	include_directories(hpl/core)
	include_directories(hpl/wdt)
	include_directories(hpl/rtc)
	include_directories(hpl/port)
	include_directories(usb)
	include_directories(usb/device)
	include_directories(usb/class/msc)
	include_directories(usb/class/msc/device)
	include_directories(samd51a/include)
	include_directories(CMSIS_5-5.0.1/CMSIS/Core/Include)
else (NOT EMULATOR_BUILD)

	add_definitions(-DEMULATOR)
	set(CMAKE_EXE_LINKER_FLAGS "-pthread")

endif (NOT EMULATOR_BUILD)

set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -Werror -Wall -Wextra -Wno-strict-aliasing -Wdouble-promotion -Wshadow -Wfloat-conversion -Wnull-dereference -Wformat=2 -std=c++17 -fno-rtti -fno-exceptions")
SET(CMAKE_CXX_FLAGS_RELEASE "-Os")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99")
SET(CMAKE_C_FLAGS_RELEASE "-Os")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

file(GLOB USER_SRC_CPP *.cpp)
file(GLOB USER_SRC_C *.c)

include_directories(.)

set(SOURCE_FILES ${SOURCE_FILES} ${USER_SRC_CPP} ${USER_SRC_C})

if (NOT EMULATOR_BUILD)
	add_executable(${PROJECT_NAME}.elf ${SOURCE_FILES})

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.map")
	set(HEX_FILE ${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.hex)
	set(BIN_FILE ${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.bin)
	add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
			COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
			COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
			COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
			COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")

	set(PROGRAM_CMD "openocd -f ${PROJECT_SOURCE_DIR}/openocd.cfg -c \"program ${PROJECT_NAME}.elf verify reset exit\"")
	install(CODE "execute_process(COMMAND ${PROGRAM_CMD})")
else (NOT EMULATOR_BUILD)

	add_executable(${PROJECT_NAME} ${SOURCE_FILES})

endif (NOT EMULATOR_BUILD)


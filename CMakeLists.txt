cmake_minimum_required(VERSION 3.13)
project(pwalk_native C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The core: the decompiled firmware plus the hardware it runs on.
add_library(pwcore STATIC
    src/api/core.c
    src/api/rom.c
    src/api/save.c

    src/hw/eeprom.c
    src/hw/lcd.c
    src/hw/machine.c

    src/firmware/debug.c
    src/firmware/handlers.c
    src/firmware/image.c
    src/firmware/ir.c
    src/firmware/loop.c
    src/firmware/sprites.c
    src/firmware/state.c
    src/firmware/text.c
    src/firmware/util.c
)
target_include_directories(pwcore PUBLIC include PRIVATE src)
target_compile_options(pwcore PRIVATE -Wall -Wextra)

# The SDL2 desktop frontend.
option(PW_BUILD_FRONTEND "Build the SDL2 frontend (pwpc)" ON)
if(PW_BUILD_FRONTEND)
    find_package(SDL2 CONFIG REQUIRED)
    add_executable(pwpc
        frontend/sdl/main.c
        frontend/sdl/files.c
        frontend/sdl/hal.c
        frontend/sdl/headless.c
        frontend/sdl/ir_udp.c
    )
    target_include_directories(pwpc PRIVATE ${SDL2_INCLUDE_DIRS})
    target_compile_options(pwpc PRIVATE -Wall -Wextra)
    target_link_libraries(pwpc pwcore ${SDL2_LIBRARIES} m)
endif()

# Tests. They need your own dumps, which are not part of this project.
option(PW_BUILD_TESTS "Build the tests" ON)
if(PW_BUILD_TESTS)
    set(PW_ROM "${CMAKE_SOURCE_DIR}/PWROM.bin" CACHE FILEPATH "Pokewalker ROM dump (48 KB)")
    set(PW_EEPROM "${CMAKE_SOURCE_DIR}/PWEEPROM.bin" CACHE FILEPATH "Pokewalker EEPROM dump (64 KB)")
    enable_testing()
    foreach(t peertest savetest)
        add_executable(${t} tests/${t}.c tests/testrom.c)
        target_include_directories(${t} PRIVATE src)
        target_link_libraries(${t} pwcore m)
        add_test(NAME ${t} COMMAND ${t} ${PW_ROM} ${PW_EEPROM})
    endforeach()
endif()

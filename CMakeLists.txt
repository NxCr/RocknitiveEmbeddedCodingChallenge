cmake_minimum_required(VERSION 3.10)
project(coding_challenge C)

set(CMAKE_C_STANDARD 99)

file(GLOB rtos_src FreeRTOS/Source/*.c)

add_executable(
        coding_challenge

        # your code files
        src/challenge.c

        # challenge code
        challenge/main.c
        challenge/emulator.c
        challenge/console.c
        challenge/run-time-stats-utils.c

        # FreeRTOS source
        ${rtos_src}

        # memory manager
        FreeRTOS/Source/portable/MemMang/heap_3.c

        # posix port
        FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
        FreeRTOS/Source/portable/ThirdParty/GCC/Posix/port.c
)

target_include_directories(
        coding_challenge
        PRIVATE
        src
        challenge
        FreeRTOS/Source/include
        FreeRTOS/Source/portable/ThirdParty/GCC/Posix
        FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils
)

target_link_libraries(coding_challenge PRIVATE pthread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(coding_challenge PUBLIC Threads::Threads)

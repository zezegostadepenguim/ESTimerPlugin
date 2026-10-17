cmake_minimum_required(VERSION 3.16)
project(ESTimerPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Runtime estática: a DLL não depende do VC++ Redistributable no PC do controlador.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/utf-8)
endif()

# Lógica portável (timer + comandos), usada pela DLL e pelos testes.
add_library(estimer_core STATIC src/Timer.cpp src/Commands.cpp)
target_include_directories(estimer_core PUBLIC src)

enable_testing()
add_executable(estimer_tests tests/test_timer.cpp)
target_link_libraries(estimer_tests PRIVATE estimer_core)
add_test(NAME estimer_tests COMMAND estimer_tests)

# DLL do EuroScope: só no Windows, 32 bits (o EuroScope é um processo x86).
if(WIN32)
    set(EUROSCOPE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk" CACHE PATH
        "Pasta com EuroScopePlugIn.h e EuroScopePlugInDll.lib")

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "O EuroScope é 32 bits: configure com -A Win32")
    endif()
    if(NOT EXISTS "${EUROSCOPE_SDK_DIR}/EuroScopePlugIn.h"
       OR NOT EXISTS "${EUROSCOPE_SDK_DIR}/EuroScopePlugInDll.lib")
        message(FATAL_ERROR
            "SDK do EuroScope não encontrado em ${EUROSCOPE_SDK_DIR}. Copie EuroScopePlugIn.h e "
            "EuroScopePlugInDll.lib da pasta PlugInEnvironment da instalação do EuroScope (ver README).")
    endif()

    add_library(ESTimerPlugin SHARED
        src/Main.cpp
        src/TimerPlugin.cpp
        src/TimerScreen.cpp)
    target_include_directories(ESTimerPlugin PRIVATE "${EUROSCOPE_SDK_DIR}")
    target_link_libraries(ESTimerPlugin PRIVATE
        estimer_core
        "${EUROSCOPE_SDK_DIR}/EuroScopePlugInDll.lib"
        user32 gdi32)
    target_compile_definitions(ESTimerPlugin PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
    if(MSVC)
        target_compile_options(ESTimerPlugin PRIVATE /W4)
    endif()
endif()

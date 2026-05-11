cmake_minimum_required(VERSION 3.15)
project(SimuladorEscalonamento)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Configurações globais ANTES do FetchContent
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # FORÇA compilação .a ao invés de .so
set(SFML_STATIC_LIBRARIES TRUE CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.x
)

set(SFML_BUILD_AUDIO FALSE CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK FALSE CACHE BOOL "" FORCE)
# Garante que o SFML não tente instalar nada no sistema
set(SFML_INSTALL FALSE CACHE BOOL "" FORCE) 

FetchContent_MakeAvailable(sfml)

# 2. Flags para embutir a biblioteca padrão do C++
add_link_options(-static-libgcc -static-libstdc++)

add_executable(simulador src/main.cpp)

# 3. No Linux, precisamos linkar as dependências de sistema manualmente 
# porque as libs do SFML agora são estáticas (.a)
if(UNIX AND NOT APPLE)
    target_link_libraries(simulador 
        sfml-graphics 
        sfml-window 
        sfml-system
        X11 
        Xcursor 
        Xrandr 
        udev 
        freetype 
        OpenGL 
        pthread
        Xrender
        Xext
        Xfixes
    )
else()
    target_link_libraries(simulador sfml-graphics sfml-window sfml-system)
endif()
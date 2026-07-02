# ============================================================================
# CMakeLists.txt — Sistema de build do Simulador de SO Multitarefa
# ============================================================================
# Base original: dupla.
# Ajustes (Diretor):
#   - GLOB recursivo para descobrir todos os .cpp em src/
#   - target_include_directories para encontrar headers em include/
#   - copia automatica dos diretorios assets/ e config/ para o build
# ============================================================================

cmake_minimum_required(VERSION 3.15)
project(SimuladorEscalonamento LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gera compile_commands.json em build/ para o IntelliSense do VSCode
# entender flags, includes e o padrao C++ usado (evita falsos-positivos
# tipo "std::filesystem nao existe").
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------------------------------------------------------
# 1. Configuracoes globais ANTES do FetchContent
# ----------------------------------------------------------------------------
# Por que ANTES? Porque o FetchContent_MakeAvailable() ja processa os
# CMakeLists internos do SFML, e essas opcoes precisam estar setadas para
# influenciar como o SFML eh compilado.
#
# BUILD_SHARED_LIBS OFF       : forca .a (estatico) ao inves de .so
# SFML_STATIC_LIBRARIES TRUE  : flag interna do SFML para configurar o link
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_STATIC_LIBRARIES TRUE CACHE BOOL "" FORCE)

# ----------------------------------------------------------------------------
# 2. Download e build da SFML via FetchContent
# ----------------------------------------------------------------------------
# Justificativa: requisito 5 do enunciado exige que o software rode
# "sem a necessidade de instalar nenhuma biblioteca extra". Compilando
# a SFML junto, o avaliador so precisa de cmake + compilador + git.
include(FetchContent)
FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.x
)

# Desabilita modulos do SFML que nao usamos (Audio, Network).
# Reduz tempo de build e tamanho do binario final.
set(SFML_BUILD_AUDIO   FALSE CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK FALSE CACHE BOOL "" FORCE)
# Impede que o build do SFML tente instalar arquivos no sistema (/usr/local/...).
set(SFML_INSTALL       FALSE CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sfml)

# ----------------------------------------------------------------------------
# 3. Flags de compilacao e link
# ----------------------------------------------------------------------------
# -static-libgcc / -static-libstdc++ : embutem a runtime do C++ no binario,
# para evitar dependencia de libstdc++.so de versao especifica da maquina alvo.
add_link_options(-static-libgcc -static-libstdc++)

# Avisos do compilador. Avisos hoje = bugs evitados na apresentacao.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ----------------------------------------------------------------------------
# 4. Coleta automatica dos arquivos-fonte
# ----------------------------------------------------------------------------
# GLOB recursivo: pega todos os .cpp dentro de src/ (e subpastas).
# CONFIGURE_DEPENDS: forca o CMake a reavaliar o glob a cada build,
# de modo que arquivos novos sejam detectados sem precisar rodar 'cmake ..'
# manualmente. Conveniencia que justifica usar GLOB aqui.
file(GLOB_RECURSE SIM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# ----------------------------------------------------------------------------
# 5. Executavel
# ----------------------------------------------------------------------------
add_executable(simulador ${SIM_SOURCES})

# Diz ao compilador onde estao nossos headers. Assim podemos usar
# #include "core/Task.hpp" em vez de caminhos relativos longos.
target_include_directories(simulador PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ----------------------------------------------------------------------------
# 6. Link das dependencias
# ----------------------------------------------------------------------------
# No Linux, como linkamos a SFML estaticamente, precisamos informar
# manualmente todas as bibliotecas de sistema que a SFML usa internamente
# (X11, OpenGL, freetype, etc). Quando o link eh dinamico, o proprio .so
# da SFML carrega isso transparentemente — mas aqui nao.
if(UNIX AND NOT APPLE)
    target_link_libraries(simulador PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        X11
        Xcursor
        Xrandr
        udev
        freetype
        GL          # OpenGL (em Linux a lib eh 'GL', nao 'OpenGL')
        pthread
        Xrender
        Xext
        Xfixes
    )
else()
    target_link_libraries(simulador PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
    )
endif()

# ----------------------------------------------------------------------------
# 7. Copia de assets e config para o diretorio de build
# ----------------------------------------------------------------------------
# Quando rodamos ./simulador a partir do diretorio build/, o programa
# procura por "assets/fonts/Roboto.ttf" e "config/exemplo.cfg" no diretorio
# corrente. Copiamos esses diretorios para o build/ para que isso funcione.
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})

# ----------------------------------------------------------------------------
# 8. Mensagem informativa pos-configuracao
# ----------------------------------------------------------------------------
message(STATUS "=====================================================")
message(STATUS "  Simulador SO — Configuracao CMake concluida")
message(STATUS "  Build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compilador    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Arquivos .cpp : ${SIM_SOURCES}")
message(STATUS "=====================================================")
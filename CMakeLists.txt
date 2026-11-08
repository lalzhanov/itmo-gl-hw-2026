cmake_minimum_required(VERSION 3.16)
project(itmo-gl-hw-2026 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets)

set(PROJECT_SOURCES
        main.cpp
        window.cpp
        window.h
        textures/cat.jpg
)

add_executable(itmo-gl-hw-2026
    ${PROJECT_SOURCES}
    resources.qrc
    shaders/fractal.fs
    shaders/fractal.vs
    openglwidget.h openglwidget.cpp
)

target_link_libraries(itmo-gl-hw-2026 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets)

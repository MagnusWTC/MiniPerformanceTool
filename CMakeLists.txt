cmake_minimum_required(VERSION 3.21)

project(MiniPerformanceTool VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    src/main.cpp
    src/SettingsManager.h
    src/SettingsManager.cpp
)

list(APPEND PROJECT_SOURCES src/WindowsMetricsProvider.h src/WindowsMetricsProvider.cpp)

list(APPEND PROJECT_SOURCES
    src/MetricsProvider.h
    src/MetricsController.h
    src/MetricsController.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/FloatingWindow.h
    src/FloatingWindow.cpp
    src/TrayController.h
    src/TrayController.cpp
    src/ThemeAnimation.h
    src/ThemeAnimation.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MiniPerformanceTool ${PROJECT_SOURCES})
else()
    add_executable(MiniPerformanceTool ${PROJECT_SOURCES})
endif()

target_link_libraries(MiniPerformanceTool PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(WIN32)
    target_compile_definitions(MiniPerformanceTool PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    set_target_properties(MiniPerformanceTool PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(MSVC)
    target_compile_options(MiniPerformanceTool PRIVATE /utf-8 /W4)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(MetricsControllerTest
        tests/MetricsControllerTest.cpp
        src/MetricsController.cpp src/MetricsController.h
        src/MetricsProvider.h
        src/WindowsMetricsProvider.cpp src/WindowsMetricsProvider.h)
    target_include_directories(MetricsControllerTest PRIVATE src)
    target_link_libraries(MetricsControllerTest PRIVATE Qt${QT_VERSION_MAJOR}::Test)
    if(MSVC)
        target_compile_options(MetricsControllerTest PRIVATE /utf-8 /W4)
    endif()
    if(WIN32)
        target_compile_definitions(MetricsControllerTest PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()
    add_test(NAME MetricsControllerTest COMMAND MetricsControllerTest)
    add_executable(WidgetBehaviorTest
        tests/WidgetBehaviorTest.cpp
        src/FloatingWindow.cpp src/FloatingWindow.h
        src/ThemeAnimation.cpp src/ThemeAnimation.h
        src/SettingsManager.cpp src/SettingsManager.h)
    target_include_directories(WidgetBehaviorTest PRIVATE src)
    target_link_libraries(WidgetBehaviorTest PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
    if(MSVC)
        target_compile_options(WidgetBehaviorTest PRIVATE /utf-8 /W4)
    endif()
    add_test(NAME WidgetBehaviorTest COMMAND WidgetBehaviorTest)
endif()

install(TARGETS MiniPerformanceTool RUNTIME DESTINATION bin)

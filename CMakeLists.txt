cmake_minimum_required(VERSION 3.16)
project(SteamGameServerLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Try Qt6 first, fall back to Qt5
find_package(Qt6 COMPONENTS Widgets Network QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Network)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# --- Main Launcher ---
set(LAUNCHER_SOURCES
    src/main.cpp
    src/UI/MainWindow.cpp
    src/Core/SteamCmdManager.cpp
    src/Core/ServerManager.cpp
    src/Core/GithubUpdater.cpp
)

set(LAUNCHER_HEADERS
    src/version.h
    src/UI/MainWindow.h
    src/Core/SteamCmdManager.h
    src/Core/ServerManager.h
    src/Core/GithubUpdater.h
)

add_executable(SteamGameServerLauncher WIN32
    ${LAUNCHER_SOURCES}
    ${LAUNCHER_HEADERS}
)

target_include_directories(SteamGameServerLauncher PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(SteamGameServerLauncher PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_compile_definitions(SteamGameServerLauncher PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

# --- Updater ---
add_executable(Updater
    updater/main.cpp
)

target_link_libraries(Updater PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(Updater PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

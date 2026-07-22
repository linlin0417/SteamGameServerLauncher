cmake_minimum_required(VERSION 3.16)
project(SteamGameServerLauncher VERSION 1.3.0 LANGUAGES C CXX)

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
    src/Core/MapPackager.cpp
    updater/miniz.c
)

set(LAUNCHER_HEADERS
    src/version.h
    src/UI/MainWindow.h
    src/Core/SteamCmdManager.h
    src/Core/ServerManager.h
    src/Core/GithubUpdater.h
    src/Core/MapPackager.h
)

add_executable(SteamGameServerLauncher WIN32
    ${LAUNCHER_SOURCES}
    ${LAUNCHER_HEADERS}
)

target_include_directories(SteamGameServerLauncher PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/updater
)
target_link_libraries(SteamGameServerLauncher PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_compile_definitions(SteamGameServerLauncher PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

# --- Updater ---
add_executable(Updater WIN32
    updater/main.cpp
    updater/miniz.c
)

# Updater is pure C/C++ with no Qt MOC usage
set_target_properties(Updater PROPERTIES AUTOMOC OFF)

target_link_libraries(Updater PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(Updater PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

# --- windeployqt ---
# Find windeployqt executable
get_target_property(_qt_bin_dir Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qt_bin_dir}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

if(WINDEPLOYQT_EXECUTABLE AND WIN32)
    # Run windeployqt on main executable
    add_custom_command(TARGET SteamGameServerLauncher POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --compiler-runtime --dir "$<TARGET_FILE_DIR:SteamGameServerLauncher>" "$<TARGET_FILE:SteamGameServerLauncher>"
        COMMENT "Running windeployqt on SteamGameServerLauncher..."
    )
    # No windeployqt needed for Updater anymore since it is pure C++
endif()

# --- Installation ---
install(TARGETS SteamGameServerLauncher Updater DESTINATION .)
install(DIRECTORY "$<TARGET_FILE_DIR:SteamGameServerLauncher>/" DESTINATION .
        FILES_MATCHING PATTERN "*.dll" PATTERN "plugins/*" PATTERN "platforms/*" PATTERN "styles/*" PATTERN "tls/*" PATTERN "networkinformation/*"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE)

# --- 安裝 SGSL 圖示 ---
install(FILES "${CMAKE_SOURCE_DIR}/src/UI/SGSL_icon.ico" DESTINATION .)

# --- CPack Configuration ---
# CPack 僅負責產出 ZIP 供自動更新使用。
# .exe 安裝檔由獨立的 NSIS 腳本 (installer/sgsl_installer.nsi) 負責產出。
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "Yukimura Saya & The Dream Studio")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Steam Game Server Launcher")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")

    # 不在 ZIP 中建立頂層目錄，讓更新程式可以直接覆蓋解壓
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
endif()

include(CPack)

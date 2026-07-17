cmake_minimum_required(VERSION 3.16)
project(SteamGameServerLauncher VERSION 1.0.2 LANGUAGES CXX)

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

# No Qt dependencies for Updater so it doesn't lock Qt DLLs
# target_link_libraries(Updater ...)

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
install(TARGETS SteamGameServerLauncher Updater DESTINATION bin)
install(DIRECTORY "$<TARGET_FILE_DIR:SteamGameServerLauncher>/" DESTINATION bin
        FILES_MATCHING PATTERN "*.dll" PATTERN "plugins/*" PATTERN "platforms/*" PATTERN "styles/*" PATTERN "tls/*" PATTERN "networkinformation/*"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE)

# --- CPack Configuration ---
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "linlin0417")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Steam Game Server Launcher")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

if(WIN32)
    # 支援產生 MSI (WIX)、EXE (NSIS) 與 ZIP
    set(CPACK_GENERATOR "WIX;NSIS;ZIP")
    set(CPACK_WIX_UPGRADE_GUID "B81B7F0E-9C3B-443B-9A74-D72D1AC5B853") # 替換為真實唯一的 GUID
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "Steam Game Server Launcher")
    
    # NSIS (EXE 安裝檔) 專屬設定，會自動產生解除安裝程式與捷徑
    set(CPACK_NSIS_DISPLAY_NAME "Steam Game Server Launcher")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/src/UI/icon.ico") # 如果未來有圖示可放這
    set(CPACK_NSIS_INSTDIR_NAME "SteamGameServerLauncher")
    
    # Shortcut for main executable
    set(CPACK_PACKAGE_EXECUTABLES "SteamGameServerLauncher" "Steam Game Server Launcher")

    # Important: Do not nest contents in a top-level directory in the ZIP file
    # so the updater can directly extract over the existing installation
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
endif()

include(CPack)

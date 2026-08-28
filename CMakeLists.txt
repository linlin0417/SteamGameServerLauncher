cmake_minimum_required(VERSION 3.16)
project(SteamGameServerLauncher VERSION 2.1.1 LANGUAGES C CXX)

# --- 版本後綴支援（方案 A：Git Tag 帶後綴） ---
# 本地開發時留空即可；CI 階段由 GitHub Actions 從 Git Tag 中提取後綴後注入
# 例如：cmake ... -DAPP_VERSION_SUFFIX="hotfix1"
set(APP_VERSION_SUFFIX "" CACHE STRING "Version suffix for non-stable builds (e.g. hotfix1, dev2, preview)")

if(APP_VERSION_SUFFIX)
    set(APP_VERSION_FULL "${PROJECT_VERSION}-${APP_VERSION_SUFFIX}")
else()
    set(APP_VERSION_FULL "${PROJECT_VERSION}")
endif()

message(STATUS "Build version: ${APP_VERSION_FULL}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# MSVC 需要明確傳遞 /Zc:__cplusplus 才能讓 Qt6 正確識別 C++17
if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8)
endif()

# Try Qt6 first, fall back to Qt5
find_package(Qt6 COMPONENTS Widgets Network QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Network)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# Qt Resource file for embedded game profiles
if(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_resources(LAUNCHER_RESOURCES src/Resources/resources.qrc)
else()
    qt5_add_resources(LAUNCHER_RESOURCES src/Resources/resources.qrc)
endif()

# --- Main Launcher ---
set(LAUNCHER_SOURCES
    src/main.cpp
    src/UI/MainWindow.cpp
    src/Core/SteamCmdManager.cpp
    src/Core/ServerManager.cpp
    src/Core/GithubUpdater.cpp
    src/Core/VersionParser.cpp
    src/Core/MapPackager.cpp
    src/Core/DiscordManager.cpp
    src/Core/GameProfile.cpp
    src/Core/GameProfileManager.cpp
    src/Core/ServerInstance.cpp
    src/Core/SettingsMigrator.cpp
    src/UI/SidebarWidget.cpp
    src/UI/ServerControlPanel.cpp
    src/UI/ServerSettingsPanel.cpp
    src/UI/SaveManagerPanel.cpp
    src/UI/GameProfileDialog.cpp
    src/UI/AboutPanel.cpp
    updater/miniz.c
)

set(LAUNCHER_HEADERS
    src/version.h
    src/UI/MainWindow.h
    src/Core/SteamCmdManager.h
    src/Core/ServerManager.h
    src/Core/GithubUpdater.h
    src/Core/VersionParser.h
    src/Core/MapPackager.h
    src/Core/DiscordManager.h
    src/Core/GameProfile.h
    src/Core/GameProfileManager.h
    src/Core/ServerInstance.h
    src/Core/SettingsMigrator.h
    src/UI/SidebarWidget.h
    src/UI/ServerControlPanel.h
    src/UI/ServerSettingsPanel.h
    src/UI/SaveManagerPanel.h
    src/UI/GameProfileDialog.h
    src/UI/AboutPanel.h
)

add_executable(SteamGameServerLauncher WIN32
    ${LAUNCHER_SOURCES}
    ${LAUNCHER_HEADERS}
    ${LAUNCHER_RESOURCES}
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
    APP_VERSION="${APP_VERSION_FULL}"
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
    APP_VERSION="${APP_VERSION_FULL}"
)

# --- Bootstrap ---
add_executable(Bootstrap WIN32
    Bootstrap/main.cpp
    Bootstrap/BootstrapWindow.cpp
    Bootstrap/BootstrapWindow.h
    src/Core/VersionParser.cpp
    src/Core/VersionParser.h
)

target_include_directories(Bootstrap PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/Bootstrap
)

target_link_libraries(Bootstrap PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

target_compile_definitions(Bootstrap PRIVATE
    APP_VERSION="${APP_VERSION_FULL}"
)

# --- windeployqt ---
# Find windeployqt executable
get_target_property(_qt_bin_dir Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qt_bin_dir}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

if(WINDEPLOYQT_EXECUTABLE AND WIN32)
    set(WINDEPLOYQT_EXTRA_ARGS "")
    if(MINGW)
        set(WINDEPLOYQT_EXTRA_ARGS "-force-openssl")
    endif()

    # Run windeployqt on main executable
    add_custom_command(TARGET SteamGameServerLauncher POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --compiler-runtime ${WINDEPLOYQT_EXTRA_ARGS} --dir "$<TARGET_FILE_DIR:SteamGameServerLauncher>" "$<TARGET_FILE:SteamGameServerLauncher>"
        COMMENT "Running windeployqt on SteamGameServerLauncher..."
    )
    # Bootstrap 共用主程式的 Qt DLL，不需要獨立執行 windeployqt
endif()

# --- Installation ---
install(TARGETS SteamGameServerLauncher Updater Bootstrap DESTINATION .)
install(DIRECTORY "$<TARGET_FILE_DIR:SteamGameServerLauncher>/" DESTINATION .
        FILES_MATCHING PATTERN "*.dll" PATTERN "plugins/*" PATTERN "platforms/*" PATTERN "styles/*" PATTERN "tls/*" PATTERN "networkinformation/*"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE)

# 產生並安裝 qt.conf 以確保外掛路徑正確載入
file(GENERATE OUTPUT "$<TARGET_FILE_DIR:SteamGameServerLauncher>/qt.conf" CONTENT "[Paths]\nPlugins=.\n")
install(FILES "$<TARGET_FILE_DIR:SteamGameServerLauncher>/qt.conf" DESTINATION .)

if(WIN32 AND MINGW)
    # 本地 MinGW 打包時，主動包入 OpenSSL 3 以確保 Qt6 TLS 初始化成功
    install(FILES 
        "C:/Program Files/MySQL/MySQL Shell 8.0/bin/libssl-3-x64.dll"
        "C:/Program Files/MySQL/MySQL Shell 8.0/bin/libcrypto-3-x64.dll"
        DESTINATION . OPTIONAL)
endif()

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

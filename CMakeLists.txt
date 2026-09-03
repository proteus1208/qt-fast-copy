cmake_minimum_required(VERSION 3.16)

project(qt-fast-copy VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui)

set(PROJECT_SOURCES
    main.cpp
    pasteitem.h
    storagemanager.h
    storagemanager.cpp
    hotkeymanager.h
    hotkeymanager.cpp
    trayicon.h
    trayicon.cpp
    settingswindow.h
    settingswindow.cpp
    settingswindow.ui
    itemeditordiag.h
    itemeditordiag.cpp
    resources.qrc
)

# Windows: embed icon into the executable via .rc file
if(WIN32)
    list(APPEND PROJECT_SOURCES app.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(qt-fast-copy
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(qt-fast-copy SHARED ${PROJECT_SOURCES})
    else()
        add_executable(qt-fast-copy ${PROJECT_SOURCES})
    endif()
endif()

target_link_libraries(qt-fast-copy PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
)

# Windows: link user32 for RegisterHotKey / SendInput (already implicit, but explicit is safer)
if(WIN32)
    target_link_libraries(qt-fast-copy PRIVATE user32)
endif()

# Linux X11 global hotkey support (kept for portability)
if(UNIX AND NOT APPLE)
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(qt-fast-copy PRIVATE ${X11_LIBRARIES})
        target_include_directories(qt-fast-copy PRIVATE ${X11_INCLUDE_DIR})
    endif()
endif()

set_target_properties(qt-fast-copy PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS qt-fast-copy
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(qt-fast-copy)
endif()

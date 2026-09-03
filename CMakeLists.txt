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
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(qt-fast-copy
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(qt-fast-copy SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(qt-fast-copy
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(qt-fast-copy PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
)

# Linux X11 global hotkey support
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(qt-fast-copy PRIVATE ${X11_LIBRARIES})
    target_include_directories(qt-fast-copy PRIVATE ${X11_INCLUDE_DIR})
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.qt-fast-copy)
endif()
set_target_properties(qt-fast-copy PROPERTIES
    ${BUNDLE_ID_OPTION}
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

cmake_minimum_required(VERSION 3.16)

project(Partorium VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME NO)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Concurrent)

set(TS_FILES Partorium_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    # App-Icon (macOS)
    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icons/Partorium_Dock.icns")

    qt_add_executable(Partorium
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${APP_ICON_MACOS}
        newpartdialog.h newpartdialog.cpp newpartdialog.ui
        part.h
        jsonpartrepository.h jsonpartrepository.cpp
        settingsdialog.h settingsdialog.cpp settingsdialog.ui
        TODO.md
        guiutils.h guiutils.cpp
        listmanagerdialog.h listmanagerdialog.cpp listmanagerdialog.ui
        importdatadialog.h importdatadialog.cpp importdatadialog.ui
        batchchangedialog.h batchchangedialog.cpp batchchangedialog.ui
    )

if(APPLE)
    # Damit die .icns ins App-Bundle unter Resources kopiert wird
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Name, den macOS im Bundle erwartet
    set_target_properties(Partorium PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "Partorium_Dock.icns"
    )
endif()

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Partorium APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(Partorium SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Partorium
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(Partorium PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Partorium)
endif()
set_target_properties(Partorium PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Partorium
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Partorium)
endif()

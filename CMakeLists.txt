cmake_minimum_required(VERSION 3.16)
project(QFiles VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Установка пути к Qt
set(CMAKE_PREFIX_PATH "W:/QT/6.9.3/mingw_64/lib/cmake")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Svg Network)

qt_standard_project_setup()

# Создаем файл ресурсов, если его нет
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/resources.qrc")
    file(WRITE "${CMAKE_SOURCE_DIR}/resources.qrc"
            "<RCC>\n"
            "    <qresource prefix=\"/\">\n"
            "        <file>QFiles.png</file>\n"
            "        <file>QFiles.ico</file>\n"
            "    </qresource>\n"
            "</RCC>\n"
    )
    message(STATUS "Created resources.qrc file")
endif()

# Создаем RC файл для иконки Windows
if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/QFiles.ico")
    file(WRITE "${CMAKE_SOURCE_DIR}/app_icon.rc"
            "IDI_ICON1 ICON DISCARDABLE \"QFiles.ico\"\n"
    )
    message(STATUS "Created app_icon.rc file for Windows icon")
endif()

# Добавляем исходные файлы
if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/app_icon.rc")
    qt_add_executable(QFiles
            main.cpp
            mainwindow.cpp
            mainwindow.h
            contextmenu.cpp
            contextmenu.h
            thumbnailview.cpp
            thumbnailview.h
            quickaccesswidget.cpp
            quickaccesswidget.h
            homepagewidget.cpp
            homepagewidget.h
            thumbnaildelegate.cpp
            thumbnaildelegate.h
            strings.h
            FileOperations.cpp
            FileOperations.h
            filesystemtab.cpp
            filesystemtab.h
            networkdrivewidget.cpp
            networkdrivewidget.h
            searchwidget.cpp
            searchwidget.h
            searchworker.cpp
            searchworker.h
            styles.h
            recyclebinwidget.cpp
            recyclebinwidget.h
            iconsizes.h
            folderviewsettings.h
            folderviewsettings.cpp
            thumbnailcache.cpp
            thumbnailcache.h
            replacefiledialog.cpp
            replacefiledialog.h
            resources.qrc
            app_icon.rc
            favoritesmenu.cpp
            favoritesmenu.h
            colors.h
            propertiesdialog.cpp
            propertiesdialog.h
            disksizeutils.cpp
            disksizeutils.h
            listmodedelegate.cpp
            listmodedelegate.h
            treesizedelegate.cpp
            treesizedelegate.h
            Archiver.cpp
            Archiver.h
            notificationwidget.cpp
            notificationwidget.h
            NotificationManager.cpp
            NotificationManager.h
            pasteworker.cpp
            pasteworker.h
            customfilesystemmodel.cpp
            customfilesystemmodel.h
    )
endif()

# Подключаем библиотеки
target_link_libraries(QFiles PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::Svg
        Qt6::Network
)

# Добавляем Windows библиотеки для работы с корзиной
if(WIN32)
    target_link_libraries(QFiles PRIVATE
            shell32
            advapi32
            ole32
            Advapi32.lib
            shlwapi
            version.lib
            wtsapi32.lib
    )
endif()

# Настройки для release/debug сборок - ВКЛЮЧАЕМ ОТЛАДОЧНЫЕ СООБЩЕНИЯ ДАЖЕ В RELEASE
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Убираем отключение отладочных сообщений
    add_definitions(-DNDEBUG)

    # Для MSVC
    if(MSVC)
        target_compile_options(QFiles PRIVATE /W4 /O2 /Ob2 /DNDEBUG)
        target_compile_definitions(QFiles PRIVATE NDEBUG)
    else()
        # Для MinGW/GCC
        target_compile_options(QFiles PRIVATE -Wall -Wextra -O2 -DNDEBUG)
        target_compile_definitions(QFiles PRIVATE NDEBUG)
    endif()
else()
    # Debug настройки
    if(MSVC)
        target_compile_options(QFiles PRIVATE /W4 /Od /Zi)
    else()
        target_compile_options(QFiles PRIVATE -Wall -Wextra -g -O0)
    endif()
endif()

# Копируем всю папку dll в выходную директорию
add_custom_command(TARGET QFiles POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/dll"
        "$<TARGET_FILE_DIR:QFiles>"
        COMMENT "Copying DLL files..."
)

# Дополнительно копируем папку platforms если она есть в dll
add_custom_command(TARGET QFiles POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/dll/platforms"
        "$<TARGET_FILE_DIR:QFiles>/platforms"
        COMMENT "Copying platform plugins..."
)

# Копируем иконки в выходную директорию (для корректного отображения)
add_custom_command(TARGET QFiles POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/QFiles.png"
        "$<TARGET_FILE_DIR:QFiles>"
        COMMENT "Copying icon files..."
)

add_custom_command(TARGET QFiles POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/QFiles.ico"
        "$<TARGET_FILE_DIR:QFiles>"
        COMMENT "Copying icon files..."
)

# Включение автоматического moc, uic и rcc
set_target_properties(QFiles PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
        WIN32_EXECUTABLE TRUE  # Для Windows приложения
)

# Настройка выходного каталога
set_target_properties(QFiles PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        OUTPUT_NAME "QFiles"  # Убедимся что имя правильное
)

# Установка свойств иконки для Windows
if(WIN32)
    set_target_properties(QFiles PROPERTIES
            RC_ICONS "${CMAKE_SOURCE_DIR}/QFiles.ico"
    )
endif()

# Добавляем информацию о версии
set_target_properties(QFiles PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
)

# Опционально: дополнительные настройки линковки для Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        set_target_properties(QFiles PROPERTIES
                LINK_FLAGS_RELEASE "/DEBUG:NONE"
        )
    else()
        set_target_properties(QFiles PROPERTIES
                LINK_FLAGS_RELEASE "-s"
        )
    endif()
endif()

# Проверка наличия иконок
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/QFiles.ico")
    message(WARNING "QFiles.ico not found in project root directory!")
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/QFiles.png")
    message(WARNING "QFiles.png not found in project root directory!")
endif()
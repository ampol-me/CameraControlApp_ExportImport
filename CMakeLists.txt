cmake_minimum_required(VERSION 3.10)
project(camera_app)

# Include the toolchain file
include(toolchain-mingw.cmake)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(JSONC REQUIRED json-c)

# Set CURL paths manually for MinGW
set(CURL_INCLUDE_DIR /opt/homebrew/opt/mingw-w64/toolchain-x86_64/include)
set(CURL_LIBRARY /opt/homebrew/opt/mingw-w64/toolchain-x86_64/lib/libcurl.dll.a)

# Add source files
set(SOURCES
    main.c
    camera.c
    http_client.c
)

# Create executable
add_executable(camera_app WIN32 ${SOURCES})

# Include directories
target_include_directories(camera_app PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${JSONC_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(camera_app PRIVATE
    ${GTK3_LIBRARIES}
    ${JSONC_LIBRARIES}
    ${CURL_LIBRARY}
    -lws2_32  # Required for Windows sockets
)

# ตั้งค่า app icon
if(APPLE)
    # macOS
    set(MACOSX_BUNDLE_ICON_FILE phila-app-icon_256.png)
    set(APP_ICON_MACOS ${CMAKE_CURRENT_SOURCE_DIR}/phila-app-icon_256.png)
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(camera_app PRIVATE ${APP_ICON_MACOS})
    set_target_properties(camera_app PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Camera Preset Controller"
        RESOURCE ${APP_ICON_MACOS}
    )
elseif(WIN32)
    # Windows
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
    target_sources(camera_app PRIVATE ${APP_ICON_RESOURCE_WINDOWS})
endif()

# Copy mock data file
if(APPLE)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mock_presets_localhost.json
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/camera_app.app/Contents/Resources)
elseif(WIN32)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mock_presets_localhost.json
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/phila-app-icon_256.ico
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/mock_presets_localhost.json
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# สำหรับ Windows: คัดลอก DLL ที่จำเป็น
if(WIN32)
    add_custom_command(TARGET camera_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GTK3_PREFIX}/bin/libgtk-3-0.dll"
            "${GTK3_PREFIX}/bin/libgdk-3-0.dll"
            "${GTK3_PREFIX}/bin/libatk-1.0-0.dll"
            "${GTK3_PREFIX}/bin/libcairo-2.dll"
            "${GTK3_PREFIX}/bin/libgdk_pixbuf-2.0-0.dll"
            "${GTK3_PREFIX}/bin/libgio-2.0-0.dll"
            "${GTK3_PREFIX}/bin/libglib-2.0-0.dll"
            "${GTK3_PREFIX}/bin/libgobject-2.0-0.dll"
            "${GTK3_PREFIX}/bin/libpango-1.0-0.dll"
            "${GTK3_PREFIX}/bin/libpangocairo-1.0-0.dll"
            "${GTK3_PREFIX}/bin/libpangoft2-1.0-0.dll"
            "${GTK3_PREFIX}/bin/libjson-c-5.dll"
            "${CMAKE_CURRENT_BINARY_DIR}"
    )
endif() 
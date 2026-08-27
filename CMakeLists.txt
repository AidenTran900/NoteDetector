cmake_minimum_required(VERSION 3.15)
project(NoteDetector)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(WIN32)
    add_definitions(-DWIN32 -D__WINDOWS_DS__)
    set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/GLFW")
    set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")

    if(MINGW)
        set(GLFW_LIBRARY "${GLFW_DIR}/lib-mingw-w64/libglfw3.a")
    else()
        set(GLFW_LIBRARY "${GLFW_DIR}/lib-vc2019/glfw3.lib")
    endif()

    set(PLATFORM_LIBS opengl32 gdi32 imm32 shell32 ole32 comdlg32 dsound winmm)
elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW glfw3)
    endif()

    if(NOT GLFW_FOUND)
        message(STATUS "GLFW not found via pkg-config, using bundled version")
        set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/GLFW")
        set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
        include_directories(${GLFW_INCLUDE_DIR})

        file(GLOB GLFW_LIBS "${GLFW_DIR}/lib-*/*.a" "${GLFW_DIR}/lib-*/*.so")
        if(GLFW_LIBS)
            list(GET GLFW_LIBS 0 GLFW_LIBRARY)
            set(GLFW_LIBRARIES ${GLFW_LIBRARY})
        else()
            message(FATAL_ERROR "No GLFW library found. Please install libglfw3-dev or build GLFW in the GLFW directory")
        endif()
    endif()

    add_definitions(-D__LINUX_ALSA__)
    find_package(ALSA REQUIRED)
    set(PLATFORM_LIBS ${ALSA_LIBRARIES} dl pthread)
elseif(APPLE)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW glfw3)
    endif()

    if(NOT GLFW_FOUND)
        message(STATUS "GLFW not found via pkg-config, using bundled version")
        set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries/GLFW")
        set(GLFW_INCLUDE_DIR "${GLFW_DIR}/include")
        include_directories(${GLFW_INCLUDE_DIR})
    endif()

    add_definitions(-D__MACOSX_CORE__)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREMIDI_LIBRARY CoreMIDI)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    set(PLATFORM_LIBS ${COREAUDIO_LIBRARY} ${COREMIDI_LIBRARY} ${COREFOUNDATION_LIBRARY} pthread)
endif()

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")
set(IMGUI_DIR "${LIBRARY_DIR}/imgui")
set(IMPLOT_DIR "${LIBRARY_DIR}/implot")
set(RTAUDIO_DIR "${LIBRARY_DIR}/rtAudio")
set(PIANO_DIR "${SRC_DIR}/piano")
set(FILEDIALOG_DIR "${LIBRARY_DIR}/filedialog")

file(GLOB IMGUI_SOURCES "${IMGUI_DIR}/*.cpp")
file(GLOB IMPLOT_SOURCES "${IMPLOT_DIR}/*.cpp")
file(GLOB RTAUDIO_SOURCES "${RTAUDIO_DIR}/*.cpp")
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")

set(SOURCES
    ${APP_SOURCES}
    ${FILEDIALOG_DIR}/tinyfiledialogs.c
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    ${RTAUDIO_SOURCES}
)

include_directories(
    ${SRC_DIR}
    ${LIBRARY_DIR}
    ${IMGUI_DIR}
    ${IMPLOT_DIR}
    ${RTAUDIO_DIR}
    ${PIANO_DIR}
    ${FILEDIALOG_DIR}
    ${OPENGL_INCLUDE_DIR}
)

if(WIN32)
    include_directories(${GLFW_INCLUDE_DIR})
else()
    include_directories(${GLFW_INCLUDE_DIRS})
endif()

add_executable(gui ${SOURCES})

if(WIN32)
    target_link_libraries(gui ${GLFW_LIBRARY} ${OPENGL_LIBRARIES} ${PLATFORM_LIBS})
else()
    target_link_libraries(gui ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} ${PLATFORM_LIBS})
endif()

if(MSVC)
    target_compile_options(gui PRIVATE /W4)
else()
    target_compile_options(gui PRIVATE -Wall -Wextra)
endif()

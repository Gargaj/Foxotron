cmake_minimum_required(VERSION 3.0)

set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
string(TIMESTAMP VERSION_PATCH "%Y%m%d")

project(Foxotron VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(CMAKE_OSX_ARCHITECTURES x86_64)

if (APPLE OR WIN32)
  set(FXTRN_EXE_NAME "Foxotron")
else ()
  set(FXTRN_EXE_NAME "Foxotron")
endif ()

if (WIN32)
  option(FOXOTRON_64BIT "Compile for 64 bit target?" ON)

  if (CMAKE_GENERATOR MATCHES "64")
    set(FOXOTRON_64BIT ON CACHE BOOL "Compile for 64 bit target?")
  else ()
    set(FOXOTRON_64BIT OFF CACHE BOOL "Compile for 64 bit target?")
  endif ()
endif ()

if (NOT (UNIX AND (NOT APPLE))) #if not linux
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
endif ()

if (APPLE)
  set(CMAKE_FIND_FRAMEWORK LAST)
endif ()

add_definitions(-DSCI_LEXER -DSCI_NAMESPACE)
if (UNIX)
  add_definitions(-DGTK)
endif ()

if (APPLE)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_CPP_EXCEPTIONS "No")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_CPP_RTTI "No")
endif ()

##############################################################################
# Global settings
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

##############################################################################
# ASSIMP
add_subdirectory(${CMAKE_SOURCE_DIR}/externals/assimp/)
set(FXTRN_PROJECT_INCLUDES ${FXTRN_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/assimp/include ${CMAKE_CURRENT_BINARY_DIR}/externals/assimp/include)
set(FXTRN_PROJECT_LIBS ${FXTRN_PROJECT_LIBS} assimp)
if (MSVC)
  target_compile_options(assimp PUBLIC "$<$<CONFIG:Release>:/MT>")
  target_compile_options(zlibstatic PUBLIC "$<$<CONFIG:Release>:/MT>")
endif ()

##############################################################################
# GLM
add_subdirectory(${CMAKE_SOURCE_DIR}/externals/glm/)
set(FXTRN_PROJECT_INCLUDES ${FXTRN_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/glm/glm)
set(FXTRN_PROJECT_LIBS ${FXTRN_PROJECT_LIBS} glm)

##############################################################################
# GLFW
# GLFW settings and project inclusion
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC OFF CACHE BOOL "" FORCE)
mark_as_advanced(BUILD_SHARED_LIBS GLFW_BUILD_EXAMPLES GLFW_BUILD_TESTS GLFW_BUILD_DOCS GLFW_INSTALL GLFW_VULKAN_STATIC)
if (UNIX)
  set(GLFW_USE_OSMESA OFF CACHE BOOL "" FORCE)
  mark_as_advanced(GLFW_USE_OSMESA)
endif()
if (WIN32)
  set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
  mark_as_advanced(USE_MSVC_RUNTIME_LIBRARY_DLL)
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/externals/glfw/)
set(FXTRN_PROJECT_INCLUDES ${FXTRN_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/glfw/include)
set(FXTRN_PROJECT_LIBS ${FXTRN_PROJECT_LIBS} glfw ${GLFW_LIBRARIES})

##############################################################################
# GLEW
set(GLEW_SRCS
  ${CMAKE_SOURCE_DIR}/externals/glew/glew.c
)
add_library(FXTRN_glew STATIC ${GLEW_SRCS})
target_include_directories(FXTRN_glew PUBLIC ${CMAKE_SOURCE_DIR}/externals/glew)
target_compile_definitions(FXTRN_glew PUBLIC -DGLEW_STATIC)
if (MSVC)
  target_compile_options(FXTRN_glew PUBLIC "$<$<CONFIG:Release>:/MT>")
endif ()
set(FXTRN_PROJECT_INCLUDES ${FXTRN_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/glew)
set(FXTRN_PROJECT_LIBS ${FXTRN_PROJECT_LIBS} FXTRN_glew)

##############################################################################
# STB
set(FXTRN_PROJECT_INCLUDES ${FXTRN_PROJECT_INCLUDES}
  ${CMAKE_SOURCE_DIR}/externals/stb
)

##############################################################################
# IMGUI
file(GLOB IMGUI_INCLUDES ${IMGUI_INCLUDES}
  ${CMAKE_SOURCE_DIR}/externals/imgui/*.h
)
file(GLOB IMGUI_SRCS
  ${CMAKE_SOURCE_DIR}/externals/imgui/*.cpp
)
set(IMGUI_INCLUDES ${IMGUI_INCLUDES}
  ${CMAKE_SOURCE_DIR}/externals/imgui/backends/imgui_impl_glfw.h
  ${CMAKE_SOURCE_DIR}/externals/imgui/backends/imgui_impl_opengl3.h
)
set(IMGUI_SRCS ${IMGUI_SRCS}
  ${CMAKE_SOURCE_DIR}/externals/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_SOURCE_DIR}/externals/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(FXTRN_ImGui STATIC ${IMGUI_SRCS})
target_include_directories(FXTRN_ImGui PUBLIC 
  ${CMAKE_SOURCE_DIR}/externals/imgui 
  ${CMAKE_SOURCE_DIR}/externals/glfw/include 
  ${CMAKE_SOURCE_DIR}/externals/glew
)
if (MSVC)
  target_compile_options(FXTRN_ImGui PUBLIC "$<$<CONFIG:Release>:/MT>")
endif ()
set(FXTRN_PROJECT_INCLUDES ${FXTRN_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/imgui)
set(FXTRN_PROJECT_LIBS ${FXTRN_PROJECT_LIBS} FXTRN_ImGui)


##############################################################################
# IMGUI ADDONS
file(GLOB IMGUIADDONS_INCLUDES
  ${CMAKE_SOURCE_DIR}/externals/imgui-addons/FileBrowser/ImGuiFileBrowser.h
)
file(GLOB IMGUIADDONS_SRCS
  ${CMAKE_SOURCE_DIR}/externals/imgui-addons/FileBrowser/ImGuiFileBrowser.cpp
)
add_library(FXTRN_ImGuiAddons STATIC ${IMGUIADDONS_SRCS})
target_include_directories(FXTRN_ImGuiAddons PUBLIC 
  ${CMAKE_SOURCE_DIR}/externals/imgui 
)
if (MSVC)
  target_compile_options(FXTRN_ImGuiAddons PUBLIC "$<$<CONFIG:Release>:/MT>")
endif ()
set(FXTRN_PROJECT_INCLUDES ${FXTRN_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/imgui-addons/FileBrowser)
set(FXTRN_PROJECT_LIBS ${FXTRN_PROJECT_LIBS} FXTRN_ImGuiAddons)

##############################################################################
# Foxotron
file(GLOB FXTRN_PROJECT_SRCS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/src/*.h
)
source_group("Foxotron" FILES ${FXTRN_PROJECT_SRCS})

set(FXTRN_PROJECT_SRCS ${FXTRN_PROJECT_SRCS} ${FXTRN_PLATFORM_SRCS} ${FXTRN_RESOURCES_DATA} ${FXTRN_CAPTURE_SRCS})

set(FXTRN_PROJECT_INCLUDES ${CMAKE_SOURCE_DIR}/src ${FXTRN_PROJECT_INCLUDES})

##############################################################################
#### APPLE BUNDLE, RESSOURCES AND DYNAMIC LIBS
if (APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)

  # Define some settings for the Bundle
  set(MACOSX_BUNDLE_BUNDLE_NAME ${FXTRN_EXE_NAME})
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "${FXTRN_EXE_NAME}")
  set(MACOSX_BUNDLE_ICON_FILE icon.icns)
  set(MACOSX_BUNDLE_INFO_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH},Copyright © 2021 The Foxotron Contributors")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2020-2021 The Foxotron Contributors. All rights reserved.")

  set_source_files_properties(${FXTRN_RESOURCES_DATA} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set_source_files_properties(${OSX_LIB_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)
  set(FXTRN_PROJECT_SRCS ${FXTRN_PROJECT_SRCS} ${OSX_LIB_FILES})

  set(FXTRN_PROJECT_SRCS ${GUI_TYPE} ${FXTRN_PROJECT_SRCS})

  find_library(COCOA_FRAMEWORK Cocoa)
  find_library(OPENGL_FRAMEWORK OpenGL)
  find_library(CARBON_FRAMEWORK Carbon)
  find_library(COREAUDIO_FRAMEWORK CoreAudio)
  find_library(AVFOUNDATION_FRAMEWORK AVFoundation)
  mark_as_advanced(COCOA_FRAMEWORK OPENGL_FRAMEWORK CARBON_FRAMEWORK COREAUDIO_FRAMEWORK AVFOUNDATION_FRAMEWORK)
  set(PLATFORM_LIBS ${COCOA_FRAMEWORK} ${OPENGL_FRAMEWORK} ${CARBON_FRAMEWORK} ${COREAUDIO_FRAMEWORK} ${AVFOUNDATION_FRAMEWORK})
elseif (UNIX)
  set(PLATFORM_LIBS GL asound fontconfig)
elseif (WIN32)
  set(PLATFORM_LIBS opengl32 glu32 winmm shlwapi)
endif ()
set(FXTRN_PROJECT_LIBS ${FXTRN_PROJECT_LIBS} ${PLATFORM_LIBS})
##############################################################################

# create the executable
link_directories(${FXTRN_LINK_DIRS})
if (UNIX AND (NOT APPLE))
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif ()
add_executable(${FXTRN_EXE_NAME} ${FXTRN_PROJECT_SRCS})
# Set compiler specific flags
if (APPLE)
  set_target_properties(${FXTRN_EXE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/data/macosx/MacOSXBundleInfo.plist.in)
elseif (UNIX AND (NOT APPLE))
  target_compile_options(${FXTRN_EXE_NAME} PUBLIC -std=c++11)
elseif (WIN32)
  if (MSVC)
    set_target_properties(${FXTRN_EXE_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
    target_compile_options(${FXTRN_EXE_NAME} PUBLIC "$<$<CONFIG:Release>:/MT>")
  endif ()
endif ()
target_include_directories(${FXTRN_EXE_NAME} PUBLIC ${FXTRN_PROJECT_INCLUDES})
target_link_libraries(${FXTRN_EXE_NAME} ${FXTRN_PROJECT_LIBS})

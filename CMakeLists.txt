cmake_minimum_required(VERSION 3.0)

set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
string(TIMESTAMP VERSION_PATCH "%Y%m%d")

project(Turntablinator VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(CMAKE_OSX_ARCHITECTURES x86_64)

if (APPLE OR WIN32)
	set(TRNT_EXE_NAME "Turntablinator")
else ()
	set(TRNT_EXE_NAME "turntablinator")
endif ()

if (WIN32)
  option(TURNTABLINATOR_64BIT "Compile for 64 bit target?" ON)

  if (CMAKE_GENERATOR MATCHES "64")
    set(TURNTABLINATOR_64BIT ON CACHE BOOL "Compile for 64 bit target?")
  else ()
    set(TURNTABLINATOR_64BIT OFF CACHE BOOL "Compile for 64 bit target?")
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
# ASSIMP
add_subdirectory(${CMAKE_SOURCE_DIR}/externals/assimp/)
set(TRNT_PROJECT_INCLUDES ${TRNT_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/assimp/include ${CMAKE_CURRENT_BINARY_DIR}/externals/assimp/include)
set(TRNT_PROJECT_LIBS ${TRNT_PROJECT_LIBS} assimp)

##############################################################################
# GLM
add_subdirectory(${CMAKE_SOURCE_DIR}/externals/glm/)
set(TRNT_PROJECT_INCLUDES ${TRNT_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/glm/glm)
set(TRNT_PROJECT_LIBS ${TRNT_PROJECT_LIBS} glm)

##############################################################################
# GLFW
# GLFW settings and project inclusion
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
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
set(TRNT_PROJECT_INCLUDES ${TRNT_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/glfw/include)
set(TRNT_PROJECT_LIBS ${TRNT_PROJECT_LIBS} glfw ${GLFW_LIBRARIES})

##############################################################################
# GLEW
set(GLEW_SRCS
  ${CMAKE_SOURCE_DIR}/externals/glew/glew.c
)
add_library(TRNT_glew STATIC ${GLEW_SRCS})
target_include_directories(TRNT_glew PUBLIC ${CMAKE_SOURCE_DIR}/externals/glew)
target_compile_definitions(TRNT_glew PUBLIC -DGLEW_STATIC)
if (MSVC)
  target_compile_options(TRNT_glew PUBLIC "$<$<CONFIG:Release>:/MT>")
endif ()
set(TRNT_PROJECT_INCLUDES ${TRNT_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/glew)
set(TRNT_PROJECT_LIBS ${TRNT_PROJECT_LIBS} TRNT_glew)

##############################################################################
# STB
set(TRNT_PROJECT_INCLUDES ${TRNT_PROJECT_INCLUDES}
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
add_library(TRNT_ImGui STATIC ${IMGUI_SRCS})
target_include_directories(TRNT_ImGui PUBLIC 
  ${CMAKE_SOURCE_DIR}/externals/imgui 
  ${CMAKE_SOURCE_DIR}/externals/glfw/include 
  ${CMAKE_SOURCE_DIR}/externals/glew
)
if (MSVC)
  target_compile_options(TRNT_ImGui PUBLIC "$<$<CONFIG:Release>:/MT>")
endif ()
set(TRNT_PROJECT_INCLUDES ${TRNT_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/externals/imgui)
set(TRNT_PROJECT_LIBS ${TRNT_PROJECT_LIBS} TRNT_ImGui)

##############################################################################
# TURNTABLINATOR
file(GLOB TRNT_PROJECT_SRCS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/src/*.h
)
source_group("Turntablinator" FILES ${TRNT_PROJECT_SRCS})

set(TRNT_PROJECT_SRCS ${TRNT_PROJECT_SRCS} ${TRNT_PLATFORM_SRCS} ${TRNT_RESOURCES_DATA} ${TRNT_CAPTURE_SRCS})

set(TRNT_PROJECT_INCLUDES ${CMAKE_SOURCE_DIR}/src ${TRNT_PROJECT_INCLUDES})

##############################################################################
#### APPLE BUNDLE, RESSOURCES AND DYNAMIC LIBS
if (APPLE)
  set(GUI_TYPE MACOSX_BUNDLE)

  # Define some settings for the Bundle
  set(MACOSX_BUNDLE_BUNDLE_NAME ${TRNT_EXE_NAME})
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "${TRNT_EXE_NAME}")
  set(MACOSX_BUNDLE_ICON_FILE icon.icns)
  set(MACOSX_BUNDLE_INFO_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH},Copyright © 2021 The Turntablinator Contributors")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2020-2021 The Turntablinator Contributors. All rights reserved.")

  set_source_files_properties(${TRNT_RESOURCES_DATA} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set_source_files_properties(${OSX_LIB_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)
  set(TRNT_PROJECT_SRCS ${TRNT_PROJECT_SRCS} ${OSX_LIB_FILES})

  set(TRNT_PROJECT_SRCS ${GUI_TYPE} ${TRNT_PROJECT_SRCS})

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
set(TRNT_PROJECT_LIBS ${TRNT_PROJECT_LIBS} ${PLATFORM_LIBS})
##############################################################################

# create the executable
link_directories(${TRNT_LINK_DIRS})
if (UNIX AND (NOT APPLE))
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif ()
add_executable(${TRNT_EXE_NAME} ${TRNT_PROJECT_SRCS})
# Set compiler specific flags
if (APPLE)
  set_target_properties(${TRNT_EXE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/data/macosx/MacOSXBundleInfo.plist.in)
elseif (UNIX AND (NOT APPLE))
  target_compile_options(${TRNT_EXE_NAME} PUBLIC -std=c++11)
elseif (WIN32)
  if (MSVC)
    set_target_properties(${TRNT_EXE_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
    target_compile_options(${TRNT_EXE_NAME} PUBLIC "$<$<CONFIG:Release>:/MT>")
  endif ()
endif ()
target_include_directories(${TRNT_EXE_NAME} PUBLIC ${TRNT_PROJECT_INCLUDES})
target_link_libraries(${TRNT_EXE_NAME} ${TRNT_PROJECT_LIBS})

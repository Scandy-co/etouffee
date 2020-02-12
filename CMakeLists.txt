# Copyright (C) 2014-2020 Scandy
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.

# For distribution

cmake_minimum_required(VERSION 3.1)

# Prevent accidentally building from the source
# This doesn't seem to do anything for me. It appears 
# to be an undocumented / unsupported "feature." 
#See https://gitlab.kitware.com/cmake/cmake/issues/18403 - @John
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}") 
  message(FATAL_ERROR "In-source builds are disabled.
    Please create a subfolder and use `cmake ..` inside it.
    NOTE: cmake will now create CMakeCache.txt and CMakeFiles/*.
          You must delete them, or cmake will refuse to work.
          
          Cleanup by running:
              $ git clean -xfd -e CMake*
  ")
endif()

if(${CMAKE_VERSION} VERSION_GREATER "3.13.0") 
  cmake_policy(SET CMP0079 NEW)
endif()

# Use clang. Must be set before project
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CC_COMPILER "clang")

project(Etouffee)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${Etouffee_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fpermissive -Werror=return-type")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")

find_package(OpenGL)
find_package(GLUT)
find_package(GLEW)
include_directories(${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})

if(GLUT_INCLUDE_DIR)
  include_directories(${GLUT_INCLUDE_DIR})
endif()

find_package(
  Roux
  NAMES Roux ScandyCore
  PATHS "$ENV{ScandyCore_DIR}" "$ENV{Roux_DIR}"
)
include_directories(${Roux_INCLUDE_DIRS})
message("Using Roux ${Roux_VERSION}")

# Make sure we've got Qt setup
if(LINUX)
  set(Qt5_DIR "/usr/lib/x86_64-linux-gnu/")
elseif(APPLE)
  set(Qt5_DIR "/usr/local/Cellar/qt/5.13.2/lib/cmake/Qt5/")
endif()
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})

# Setup the Qt project
set(project_sources ${Etouffee_SOURCE_DIR}/main.cpp ${Etouffee_SOURCE_DIR}/mainwindow.cpp)
set(project_headers ${Etouffee_SOURCE_DIR}/mainwindow.h)
set(project_ui ${Etouffee_SOURCE_DIR}/mainwindow.ui)

include_directories(${Etouffee_SOURCE_DIR} ${Etouffee_BINARY_DIR})

# wrap the ui file to a c++ header
qt5_wrap_ui(ui_wrap ${project_ui})
qt5_wrap_cpp(moc_sources ${project_headers})

# Define the path to dependencies dir
add_definitions(-DDEPS_DIR_PATH="${Etouffee_SOURCE_DIR}/dependencies/")

# Setup the Etouffee App
if( APPLE )
  # Roux for macOS is currently not shipping with OpenCV
  find_package(OpenCV)
  include_directories(${OpenCV_INCLUDE_DIRS})

  # make a MACOSX_BUNDLE so we can use this with Xcode
  add_executable(EtouffeeApp MACOSX_BUNDLE
    ${project_sources}
    ${project_header}
    ${ui_wrap}
    ${moc_sources}
  )
  # Link the special apple sauce
  set_target_properties(
    EtouffeeApp
    PROPERTIES COMPILE_FLAGS "-x objective-c++"
  )
  set(APPLE_LIBS
    "compression"
    "-framework Foundation"
    "-framework OpenGL"
    "-framework GLKit"
    "-framework CoreGraphics"
    "-framework CoreVideo"
  )
  list(APPEND APPLE_LIBS "-framework Metal")
  target_link_libraries(EtouffeeApp ${APPLE_LIBS})

  # Set the executable path APPLE
  set(QMAKE_LFLAGS_SONAME="-Wl,-install_name,@executable_path/")
else()
  find_package(OpenCL REQUIRED )
  include_directories(${OpenCL_INCLUDE_DIRS})

  add_executable(EtouffeeApp
    ${project_sources}
    ${project_header}
    ${ui_wrap}
    ${moc_sources}
  )
  target_link_libraries( EtouffeeApp
    ${OpenCL_LIBRARIES}
    ${OPENGL_LIBRARIES}
  )
endif()

# Link the executable to all the things
target_link_libraries( EtouffeeApp
  Qt5::Core
  Qt5::Widgets
  Qt5::OpenGL
  ${Roux_LIBS}
)

# Setup package and installation 
include(Distribution)

install(
  TARGETS EtouffeeApp
  DESTINATION .
)
install(
  FILES ${Roux_LIBS}
  DESTINATION lib
)
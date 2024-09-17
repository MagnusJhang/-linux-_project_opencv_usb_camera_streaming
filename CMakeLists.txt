cmake_minimum_required(VERSION 3.10)

# Set the project name
project([linux]_project_opencv_usb_camera_streaming)

set( CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin )
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR} )

# Find the OpenCV package
find_package(OpenCV REQUIRED)

# Add the executable
add_executable(usb_camera src/main.cpp)

# Link the OpenCV libraries
target_link_libraries(usb_camera ${OpenCV_LIBS})

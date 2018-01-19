# Set the name and the supported language of the project
PROJECT(hello-world C)
# Set the minimum version of cmake required to build this project
CMAKE_MINIMUM_REQUIRED(VERSION 3.8)

# Use the package PkgConfig to detect GTK+ headers/library files
FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)

# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})

# Add other flags to the compiler
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})

# Add an executable compiled from hello.c
ADD_EXECUTABLE(client  app/includes/client.h app/includes/prepares.h app/includes/text_buffer_handler.h app/includes/bindings.h app/includes/common.h app/client.c)
ADD_EXECUTABLE(server app/includes/text_handler.h app/includes/common.h app/server.c)

# Link the target to the GTK+ libraries
TARGET_LINK_LIBRARIES(client ${GTK3_LIBRARIES} -lpthread -rdynamic)
#TARGET_LINK_LIBRARIES(version ${GTK3_LIBRARIES})
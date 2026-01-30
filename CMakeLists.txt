cmake_minimum_required(VERSION 3.16)
project(demo)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# Create your game executable target as usual
add_executable(demo demo.c cmidi.c cmidiprograms.c)

# Link to the actual SDL3 library.
target_link_libraries(demo 
	PRIVATE m
)

target_compile_features(demo PRIVATE c_std_11)


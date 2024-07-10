

add_library(util INTERFACE)
target_include_directories(util INTERFACE .)

# Specify include directories for util target
# target_include_directories(util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
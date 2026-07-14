cmake_minimum_required(VERSION 3.10)
project(damagedetector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(damagedetector src/cpp/main.cpp src/cpp/shortcut.h)

set_target_properties(damagedetector PROPERTIES PREFIX "")
set_target_properties(damagedetector PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(WIN32)
    target_link_libraries(damagedetector PRIVATE 
        user32 
        gdi32 
        wsock32
        ws2_32
        ole32
        shell32
    )
    
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(damagedetector PRIVATE -static)
    endif()
endif()
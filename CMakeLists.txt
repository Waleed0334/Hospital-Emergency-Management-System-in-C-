
cmake_minimum_required(VERSION 3.21)

project(HospitalManagementSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)

qt_add_executable(HospitalManagementSystem
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp

    src/hospital_types.h

    src/hospital_storage.h
    src/hospital_storage.cpp

    src/hospital_engine.h
    src/hospital_engine.cpp
)

target_link_libraries(HospitalManagementSystem
    PRIVATE
        Qt6::Widgets
)

if(WIN32)
    set_target_properties(HospitalManagementSystem PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

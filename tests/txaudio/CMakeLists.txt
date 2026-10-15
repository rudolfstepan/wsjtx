cmake_minimum_required(VERSION 3.16)
project(txaudio_checks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets Network Multimedia Test)
add_executable(txaudio_checks txaudio_checks.cpp
  ../../Modulator/Modulator.cpp ../../Audio/AudioDevice.cpp ../../Audio/soundout.cpp
  ../../Transceiver/Transceiver.cpp)
target_include_directories(txaudio_checks PRIVATE ../..)
target_link_libraries(txaudio_checks PRIVATE Qt5::Widgets Qt5::Network Qt5::Multimedia Qt5::Test)
target_compile_options(txaudio_checks PRIVATE -Wall -Wextra -Werror)
if(WIN32)
  target_compile_definitions(txaudio_checks PRIVATE _WIN32_WINNT=0x0602 WINVER=0x0602)
endif()
enable_testing()
add_test(NAME txaudio_checks COMMAND txaudio_checks -o "${CMAKE_CURRENT_BINARY_DIR}/test-results.txt,txt")
set_tests_properties(txaudio_checks PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../..")

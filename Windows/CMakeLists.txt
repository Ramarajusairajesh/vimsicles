cmake_minimum_required(VERSION 3.10)
project(vimsicles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)

# Receiver executable
add_executable(receiver file_recieve.cpp)
target_link_libraries(receiver PRIVATE OpenSSL::SSL OpenSSL::Crypto ws2_32)

# Sender executable
add_executable(sender file_send.cpp)
target_link_libraries(sender PRIVATE OpenSSL::SSL OpenSSL::Crypto ws2_32) 
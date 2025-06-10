cmake_minimum_required(VERSION 3.10)
project(vimsicles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Add sender executable
add_executable(sender file_send.cpp)
target_link_libraries(sender PRIVATE ws2_32 shell32 OpenSSL::SSL OpenSSL::Crypto)

# Add receiver executable
add_executable(receiver file_recieve.cpp)
target_link_libraries(receiver PRIVATE ws2_32 OpenSSL::SSL OpenSSL::Crypto) 
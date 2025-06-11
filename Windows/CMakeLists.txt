cmake_minimum_required(VERSION 3.10)
project(vimsicles)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)

# Try to find nlohmann_json, but make it optional for the receiver
find_package(nlohmann_json QUIET)

# Receiver executable
add_executable(receiver file_recieve.cpp)
target_link_libraries(receiver PRIVATE OpenSSL::SSL OpenSSL::Crypto ws2_32)

# Sender executable
add_executable(sender file_send.cpp)
target_link_libraries(sender PRIVATE OpenSSL::SSL OpenSSL::Crypto ws2_32)

# Only link nlohmann_json if it was found
if(nlohmann_json_FOUND)
    target_link_libraries(sender PRIVATE nlohmann_json::nlohmann_json)
else()
    message(WARNING "nlohmann_json not found. Sender functionality may be limited.")
endif() 
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/md5.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 65536
#define DEFAULT_PORT 8080

std::string calculateMD5(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for MD5 calculation");
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    char buffer[BUFFER_SIZE];
    while (file.good()) {
        file.read(buffer, BUFFER_SIZE);
        MD5_Update(&md5Context, buffer, file.gcount());
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);

    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

std::string getHomeDirectory() {
    char* home = getenv("HOME");
    if (!home) {
        home = getenv("USERPROFILE");
    }
    if (!home) {
        throw std::runtime_error("Could not determine home directory");
    }
    return std::string(home);
}

void extractArchive(const std::string& archivePath, const std::string& extractPath) {
    std::string command = "tar -xzf " + archivePath + " -C " + extractPath;
    if (system(command.c_str()) != 0) {
        throw std::runtime_error("Failed to extract archive");
    }
}

int main(int argc, char* argv[]) {
    int port = (argc > 1) ? std::stoi(argv[1]) : DEFAULT_PORT;

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Bind socket
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Waiting for connection on port " << port << "..." << std::endl;

    // Accept connection
    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    try {
        // Receive file info
        char buffer[BUFFER_SIZE] = {0};
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived == SOCKET_ERROR) {
            throw std::runtime_error("Failed to receive file info");
        }

        std::string fileInfo(buffer);
        size_t separatorPos = fileInfo.find('|');
        if (separatorPos == std::string::npos) {
            throw std::runtime_error("Invalid file info format");
        }

        std::string filename = fileInfo.substr(0, separatorPos);
        std::string expectedMD5 = fileInfo.substr(separatorPos + 1);

        // Send hello response
        const char* response = "hello";
        if (send(clientSocket, response, strlen(response), 0) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send response");
        }

        // Create download directory
        std::string downloadDir = getHomeDirectory() + "/Downloads/vimsicles";
        std::filesystem::create_directories(downloadDir);

        // Receive file
        std::string filepath = downloadDir + "/" + filename;
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot create file for writing");
        }

        while (true) {
            bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytesReceived <= 0) break;
            file.write(buffer, bytesReceived);
        }

        file.close();

        // Verify MD5
        std::string actualMD5 = calculateMD5(filepath);
        if (actualMD5 != expectedMD5) {
            std::filesystem::remove(filepath);
            throw std::runtime_error("MD5 verification failed");
        }

        // Extract archive
        extractArchive(filepath, downloadDir);
        std::filesystem::remove(filepath);

        std::cout << "File received and extracted successfully!" << std::endl;

        // Cleanup
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
}

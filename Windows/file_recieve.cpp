#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 65536
#define DEFAULT_PORT 8080

std::string calculateMD5(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for MD5 calculation");
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create MD5 context");
    }

    if (!EVP_DigestInit_ex(ctx, EVP_md5(), nullptr)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize MD5 context");
    }

    char buffer[BUFFER_SIZE];
    while (file.good()) {
        file.read(buffer, BUFFER_SIZE);
        if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to update MD5 context");
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    if (!EVP_DigestFinal_ex(ctx, digest, &digest_len)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize MD5 context");
    }

    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < digest_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

std::string getHomeDirectory() {
    char* home = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&home, &sz, "HOME") != 0 || home == nullptr) {
        if (_dupenv_s(&home, &sz, "USERPROFILE") != 0 || home == nullptr) {
            throw std::runtime_error("Could not determine home directory");
        }
    }
    std::string result(home);
    free(home);
    return result;
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

        // Debug logging
        std::cout << "Received " << bytesReceived << " bytes" << std::endl;
        std::cout << "Raw buffer contents (hex): ";
        for (int i = 0; i < bytesReceived; i++) {
            printf("%02X ", (unsigned char)buffer[i]);
        }
        std::cout << std::endl;

        // Ensure null termination
        buffer[bytesReceived] = '\0';

        // Convert buffer to string and remove any trailing whitespace
        std::string fileInfo(buffer);
        std::cout << "Raw string before trimming: '" << fileInfo << "'" << std::endl;
        fileInfo.erase(fileInfo.find_last_not_of(" \n\r\t") + 1);
        std::cout << "String after trimming: '" << fileInfo << "'" << std::endl;

        // Parse metadata: type|filename|md5
        size_t firstSeparator = fileInfo.find('|');
        size_t secondSeparator = fileInfo.find('|', firstSeparator + 1);
        
        if (firstSeparator == std::string::npos || secondSeparator == std::string::npos) {
            std::cerr << "Invalid metadata format. Separators not found." << std::endl;
            std::cerr << "First separator position: " << firstSeparator << std::endl;
            std::cerr << "Second separator position: " << secondSeparator << std::endl;
            throw std::runtime_error("Invalid file info format");
        }

        std::string type = fileInfo.substr(0, firstSeparator);
        std::string filename = fileInfo.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1);
        std::string expectedMD5 = fileInfo.substr(secondSeparator + 1);

        std::cout << "Parsed metadata:" << std::endl;
        std::cout << "  Type: '" << type << "'" << std::endl;
        std::cout << "  Filename: '" << filename << "'" << std::endl;
        std::cout << "  MD5: '" << expectedMD5 << "'" << std::endl;

        // Send HELLO response (matching Android's expected format)
        const char* response = "HELLO\n";
        std::cout << "Sending response: '" << response << "'" << std::endl;
        if (send(clientSocket, response, static_cast<int>(strlen(response)), 0) == SOCKET_ERROR) {
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

        // Handle based on type
        if (type == "folder") {
            // Extract archive
            extractArchive(filepath, downloadDir);
            std::filesystem::remove(filepath);
            std::cout << "Folder received and extracted successfully!" << std::endl;
        } else if (type == "file") {
            // Move file to final location
            std::string finalPath = downloadDir + "/" + filename;
            std::filesystem::rename(filepath, finalPath);
            std::cout << "File received successfully!" << std::endl;
        } else {
            throw std::runtime_error("Unknown file type");
        }

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

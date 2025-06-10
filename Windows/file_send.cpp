#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/md5.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

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

std::string selectFiles() {
    BROWSEINFOW bi = { 0 };
    bi.lpszTitle = L"Select folder to share";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.hwndOwner = NULL;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl == nullptr) {
        return "";
    }

    wchar_t path[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl, path)) {
        CoTaskMemFree(pidl);
        return "";
    }

    CoTaskMemFree(pidl);
    
    // Convert wide string to UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, path, -1, &result[0], size_needed, NULL, NULL);
    result.pop_back(); // Remove null terminator
    return result;
}

std::string createArchive(const std::string& folderPath) {
    std::string archiveName = "shared_files.tar.gz";
    std::string command = "tar -czf " + archiveName + " -C " + 
                         folderPath.substr(0, folderPath.find_last_of("/\\")) + " " +
                         std::filesystem::path(folderPath).filename().string();
    
    if (system(command.c_str()) != 0) {
        throw std::runtime_error("Failed to create archive");
    }
    
    return archiveName;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ip_address> [port]" << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    int port = (argc > 2) ? std::stoi(argv[2]) : DEFAULT_PORT;

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Connect to server
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Connection failed" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    try {
        // Select files and create archive
        std::string folderPath = selectFiles();
        if (folderPath.empty()) {
            std::cerr << "No folder selected" << std::endl;
            return 1;
        }

        std::string archiveName = createArchive(folderPath);
        std::string md5Hash = calculateMD5(archiveName);

        // Send file info
        std::string fileInfo = archiveName + "|" + md5Hash;
        if (send(sock, fileInfo.c_str(), static_cast<int>(fileInfo.length()), 0) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send file info");
        }

        // Wait for hello response
        char response[6] = {0};
        if (recv(sock, response, 5, 0) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to receive response");
        }

        if (std::string(response) != "hello") {
            throw std::runtime_error("Invalid response from server");
        }

        // Send file
        std::ifstream file(archiveName, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open archive for sending");
        }

        char buffer[BUFFER_SIZE];
        while (file.good()) {
            file.read(buffer, BUFFER_SIZE);
            if (send(sock, buffer, static_cast<int>(file.gcount()), 0) == SOCKET_ERROR) {
                throw std::runtime_error("Failed to send file data");
            }
        }

        // Cleanup
        file.close();
        std::filesystem::remove(archiveName);
        closesocket(sock);
        WSACleanup();

        std::cout << "File sent successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
}

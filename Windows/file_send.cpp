#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/md5.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")

#define BUFFER_SIZE 65536
#define DEFAULT_PORT 8080

using json = nlohmann::json;

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

std::vector<std::string> selectFiles() {
    std::vector<std::string> selectedPaths;
    
    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return selectedPaths;
    }

    // Create file dialog
    IFileOpenDialog* pFileOpen = NULL;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                         IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    
    if (SUCCEEDED(hr)) {
        // Set options
        DWORD dwOptions;
        pFileOpen->GetOptions(&dwOptions);
        pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);

        // Show the dialog
        hr = pFileOpen->Show(NULL);
        
        if (SUCCEEDED(hr)) {
            // Get the results
            IShellItemArray* pItems;
            hr = pFileOpen->GetResults(&pItems);
            
            if (SUCCEEDED(hr)) {
                DWORD count;
                pItems->GetCount(&count);
                
                for (DWORD i = 0; i < count; i++) {
                    IShellItem* pItem;
                    hr = pItems->GetItemAt(i, &pItem);
                    
                    if (SUCCEEDED(hr)) {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                        
                        if (SUCCEEDED(hr)) {
                            // Convert wide string to UTF-8
                            int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                            std::string result(size_needed, 0);
                            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &result[0], size_needed, NULL, NULL);
                            result.pop_back(); // Remove null terminator
                            selectedPaths.push_back(result);
                            
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
                pItems->Release();
            }
        }
        pFileOpen->Release();
    }
    
    CoUninitialize();
    return selectedPaths;
}

std::string createArchive(const std::vector<std::string>& paths) {
    std::string archiveName = "shared_files.tar.gz";
    std::string command = "tar -czf " + archiveName;
    
    // Add each file/folder to the archive
    for (const auto& path : paths) {
        command += " \"" + path + "\"";
    }
    
    if (system(command.c_str()) != 0) {
        throw std::runtime_error("Failed to create archive");
    }
    
    return archiveName;
}

json createMetadata(const std::vector<std::string>& paths, const std::string& archiveName, const std::string& md5Hash) {
    json metadata;
    metadata["archive_name"] = archiveName;
    metadata["md5_hash"] = md5Hash;
    metadata["total_files"] = paths.size();
    metadata["files"] = json::array();

    for (const auto& path : paths) {
        json fileInfo;
        fileInfo["path"] = path;
        fileInfo["name"] = std::filesystem::path(path).filename().string();
        fileInfo["is_directory"] = std::filesystem::is_directory(path);
        fileInfo["size"] = std::filesystem::is_directory(path) ? 0 : std::filesystem::file_size(path);
        fileInfo["last_modified"] = std::filesystem::last_write_time(path).time_since_epoch().count();
        metadata["files"].push_back(fileInfo);
    }

    return metadata;
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
        std::vector<std::string> selectedPaths = selectFiles();
        if (selectedPaths.empty()) {
            std::cerr << "No files selected" << std::endl;
            return 1;
        }

        std::string archiveName = createArchive(selectedPaths);
        std::string md5Hash = calculateMD5(archiveName);

        // Create and send metadata
        json metadata = createMetadata(selectedPaths, archiveName, md5Hash);
        std::string metadataStr = metadata.dump();
        
        // Send metadata length first
        uint32_t metadataLength = static_cast<uint32_t>(metadataStr.length());
        if (send(sock, reinterpret_cast<char*>(&metadataLength), sizeof(metadataLength), 0) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send metadata length");
        }

        // Send metadata
        if (send(sock, metadataStr.c_str(), static_cast<int>(metadataStr.length()), 0) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to send metadata");
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

        std::cout << "Files sent successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }
}

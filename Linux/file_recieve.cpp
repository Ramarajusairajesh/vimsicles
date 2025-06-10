#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <filesystem>
#include <sstream>
#include <cstdlib>

#define Chunks_size 65536
#define DEFAULT_PORT 8080

using namespace std;
namespace fs = std::filesystem;

class receiver {
private:
    string receive_metadata(int sock) {
        char buffer[1024] = {0};
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            throw runtime_error("Failed to receive metadata");
        }
        buffer[bytes_received] = '\0';
        return string(buffer);
    }

    void send_response(int sock, const string& response) {
        if (send(sock, response.c_str(), response.length(), 0) < 0) {
            throw runtime_error("Failed to send response");
        }
    }

    bool verify_md5(const string& filename, const string& expected_md5) {
        string cmd = "md5sum " + filename + " | awk '{print $1}'";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            throw runtime_error("Failed to calculate MD5 hash");
        }

        char buffer[33] = {0};
        if (fgets(buffer, sizeof(buffer), pipe) == NULL) {
            pclose(pipe);
            throw runtime_error("Failed to read MD5 hash");
        }
        pclose(pipe);
        buffer[32] = '\0'; // Remove newline

        return string(buffer) == expected_md5;
    }

    void extract_archive(const string& archive_path) {
        // Create the target directory if it doesn't exist
        string target_dir = string(getenv("HOME")) + "/Downloads/vimsicles";
        fs::create_directories(target_dir);

        // Extract the archive
        string cmd = "tar -xzf " + archive_path + " -C " + target_dir;
        if (system(cmd.c_str()) != 0) {
            throw runtime_error("Failed to extract archive");
        }
    }

public:
    int port;

    receiver(int p) : port(p) {}

    int initialize() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            cerr << "Error creating socket" << endl;
            return 1;
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            cerr << "Error setting socket options" << endl;
            close(server_fd);
            return 1;
        }

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            cerr << "Error binding socket" << endl;
            close(server_fd);
            return 1;
        }

        if (listen(server_fd, 3) < 0) {
            cerr << "Error listening on socket" << endl;
            close(server_fd);
            return 1;
        }

        cout << "Waiting for connection on port " << port << "..." << endl;

        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            cerr << "Error accepting connection" << endl;
            close(server_fd);
            return 1;
        }

        try {
            // Receive metadata (filename and MD5 hash)
            string metadata = receive_metadata(client_socket);
            size_t pos = metadata.find('|');
            if (pos == string::npos) {
                throw runtime_error("Invalid metadata format");
            }

            string filename = metadata.substr(0, pos);
            string expected_md5 = metadata.substr(pos + 1);

            // Send acknowledgment
            send_response(client_socket, "hello");

            // Receive the file
            ofstream file(filename, ios::binary);
            if (!file) {
                throw runtime_error("Failed to create file");
            }

            char buffer[Chunks_size];
            while (true) {
                int bytes_received = recv(client_socket, buffer, Chunks_size, 0);
                if (bytes_received <= 0) break;
                file.write(buffer, bytes_received);
            }
            file.close();

            // Verify MD5 hash
            if (!verify_md5(filename, expected_md5)) {
                throw runtime_error("MD5 hash verification failed");
            }

            // Extract the archive
            extract_archive(filename);

            // Clean up the archive file
            fs::remove(filename);

            cout << "File received, verified, and extracted successfully" << endl;
        }
        catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            close(client_socket);
            close(server_fd);
            return 1;
        }

        close(client_socket);
        close(server_fd);
        return 0;
    }
};

int main(int argc, char** argv) {
    int port = DEFAULT_PORT;
    
    if (argc > 2) {
        cout << "Usage: " << argv[0] << " [port]" << endl;
        cout << "If no port is specified, default port " << DEFAULT_PORT << " will be used." << endl;
        return 1;
    }
    
    if (argc == 2) {
        port = stoi(argv[1]);
    }
    
    cout << "Using port: " << port << endl;
    receiver server(port);
    return server.initialize();
}

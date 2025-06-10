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
#include <sstream>
#include <cstdlib>

#define Chunks_size 65536

using namespace std;
// The person send the file acts as a client who sends data
// while the reciever acts as a server to recieve those files
// Generate md5 hashes for reliability
// no compressions since compressions require more cpu headroom
class sender
{

      private:
        int handshake(int sock, const string& filename, const string& md5hash)
        {
                // Send filename and MD5 hash
                string metadata = filename + "|" + md5hash;
                if (send(sock, metadata.c_str(), metadata.length(), 0) < 0)
                {
                        cerr << "Failed to send metadata" << endl;
                        close(sock);
                        return 1;
                }

                // Wait for server response
                char response[10] = {0};
                int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
                if (bytes_received <= 0)
                {
                        cerr << "Failed to receive server response" << endl;
                        close(sock);
                        return 1;
                }

                response[bytes_received] = '\0';
                if (string(response) != "hello")
                {
                        cerr << "Server rejected the transfer" << endl;
                        close(sock);
                        return 1;
                }

                cout << "Server accepted the transfer. Starting file transfer..." << endl;
                return 0;
        }

      public:
        string client_ip;
        int port;
        string archive_path;

        sender(string ip, int p, string path) : client_ip(ip), port(p), archive_path(path) {}
        int initialize()
        {
                const char *sender_ip = client_ip.c_str();
                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0)
                {
                        cerr << "Error creating socket" << endl;
                        return 1;
                }

                struct sockaddr_in server_addr;
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port   = htons(port);

                if (inet_pton(AF_INET, sender_ip, &server_addr.sin_addr) <= 0)
                {
                        cerr << "Invalid address" << endl;
                        close(sock);
                        return 1;
                }

                if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                        cerr << "Connection failed" << endl;
                        close(sock);
                        return 1;
                }

                // Calculate MD5 hash of the file
                string cmd = "md5sum " + archive_path + " | awk '{print $1}'";
                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                        cerr << "Failed to calculate MD5 hash" << endl;
                        close(sock);
                        return 1;
                }

                char md5hash[33] = {0};
                if (fgets(md5hash, sizeof(md5hash), pipe) == NULL)
                {
                        cerr << "Failed to read MD5 hash" << endl;
                        pclose(pipe);
                        close(sock);
                        return 1;
                }
                pclose(pipe);
                md5hash[32] = '\0'; // Remove newline

                // Get filename from path
                string filename = archive_path.substr(archive_path.find_last_of("/\\") + 1);

                int status = handshake(sock, filename, md5hash);
                if (status == 1)
                {
                        cerr << "Handshake failed" << endl;
                        return 1;
                }

                send_data(sock);
                close(sock);
                return 0;
        }

        int send_data(int sock)
        {
                ifstream file(archive_path, ios::binary);
                if (!file)
                {
                        cerr << "Error opening file" << endl;
                        close(sock);
                        return 1;
                }

                char buffer[Chunks_size];
                while (file)
                {
                        file.read(buffer, Chunks_size);
                        streamsize bytes_read = file.gcount();
                        if (bytes_read > 0)
                        {
                                ssize_t bytes_sent = send(sock, buffer, bytes_read, 0);
                                if (bytes_sent < 0)
                                {
                                        cerr << "Error sending data" << endl;
                                        break;
                                }
                        }
                }

                file.close();
                cout << "File sent successfully" << endl;
                return 0;
        }
};

void creating_archive(char **files) {}

int main(int argc, char **argv)
{
        if (argc != 3)
        {
                cout << "Usage: " << argv[0] << " <ip_address> <port>" << endl;
                return 1;
        }

        string ip = argv[1];
        int port = stoi(argv[2]);

        // Run the archive creation script
        FILE *pipe = popen("./creating_archive.sh", "r");
        if (!pipe)
        {
                cerr << "Failed to run archive creation script" << endl;
                return 1;
        }

        char buffer[256];
        string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
        {
                result += buffer;
        }
        pclose(pipe);

        if (result.empty())
        {
                cerr << "No archive was created" << endl;
                return 1;
        }

        // Parse the archive name and MD5 hash
        size_t pos = result.find('|');
        if (pos == string::npos)
        {
                cerr << "Invalid output from archive script" << endl;
                return 1;
        }

        string archive_name = result.substr(0, pos);
        string md5_hash = result.substr(pos + 1);
        md5_hash = md5_hash.substr(0, 32); // Remove newline

        sender client(ip, port, archive_name);
        return client.initialize();
}

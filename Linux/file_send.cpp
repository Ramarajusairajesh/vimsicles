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

#define Chunks_size 65536

using namespace std;
// The person send the file acts as a client who sends data
// while the reciever acts as a server to recieve those files
// Generate sha256 hashes for reliability
// reciever
// no compressions since compressions require more cpu headroom
// reciever
class sender
{

      private:
        int handshake(int sock)
        {
                // 1 st stage of the handshake
                // Intialise the first connection to the server
                const char *message = "hello";
                if (send(sock, message, strlen(message), 0) < 0)
                {
                        cerr << "Connection success but failed to initialize the handshake" << endl;
                        close(sock);
                        return 1;
                }
                // 2nd stage of the handshake
                // waiting for server respose
                char message_buffer[10] = {0};
                int bytes_recieved      = recv(sock, message_buffer, sizeof(message_buffer) - 1, 0);
                if (bytes_recieved <= 0)
                {
                        cerr << "Failed to recieve the message\nHandshake failed" << endl;
                        close(sock);
                        return 1;
                }
                // 3rd stage of the handshake
                // Ready to send data
                message_buffer[bytes_recieved] = '\0';
                cout << "Connection successfully established ready to send data" << endl;

                return 0;
        }

      public:
        string &client_ip;
        int port;
        string &archive_path;

        sender(string &ip, int p, string &path) : client_ip(ip), archive_path(path), port(p) {}
        int intialise()

        {
                const char *sender_ip = client_ip.c_str();
                int sock, opt = 1;
                sock = socket(AF_INET, SOCK_STREAM, 0);
                struct sockaddr_in server_addr;
                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port   = htons(port);
                if (sock < 0)
                {
                        cerr << ("Error while creating a socket") << endl;
                        return 1;
                }
                if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
                {

                        cerr << "setsockopt failed to enable reusability" << endl;
                        close(sock);
                        return 1;
                }
                if (inet_pton(AF_INET, sender_ip, &server_addr.sin_addr) <= 0)

                {
                        cerr << "Invalide address or address not supported" << endl;
                        close(sock);
                        return 1;
                }
                if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                {
                        cerr << "Connection failed/ Time out" << endl;
                        return 1;
                }

                int status = handshake(sock);

                if (status == 1)
                {
                        cerr << " Error can't send file(s)" << endl;
                        return 1;
                }
                send_data(sock);
                close(sock);
                return 0;
        }

        int send_data(int sock)
        {
                // read the compressed archive
                ifstream file(archive_path, ios::binary);
                if (!file)
                {
                        cerr << "Error reading the archive / file , check read and write "
                                "permission for both current directory and the file"
                             << endl;
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
                                ssize_t bytes_send = (sock, buffer, bytes_read, 0);
                                if (bytes_send < 0)
                                {
                                        cerr << "Error no data recieved";
                                        break;
                                }
                        }
                }
                file.close();
                cout << "File(s) are send successfully";
                return 0;
        }
};

int main(int argc, char **argv)
{
        int port            = 9080;
        string user         = argv[1];
        string archive_path = argv[2];
        if (argc < 3)
        {
                cout << "Required atleast 2 arguments to work but the program only got :" << argc
                     << endl;
                return 1;
        }
        if (argc == 4)
        {
                port = stoi(argv[3]);
        }
        for (char &c : user)
        {
                c = tolower(static_cast<unsigned char>(c));
        }
        sender client(user, port, archive_path);

        return 0;
}

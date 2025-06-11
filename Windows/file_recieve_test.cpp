#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <winsock2.h>
#include <windows.h>
#include <openssl/md5.h>

#pragma comment(lib, "ws2_32.lib")

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class FileReceiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Winsock
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    void TearDown() override {
        // Cleanup Winsock
        WSACleanup();
    }
};

TEST_F(FileReceiveTest, ReceiveMetadata) {
    // Create a temporary file to simulate the received archive
    std::string tempArchive = "test_archive.tar.gz";
    std::ofstream archiveFile(tempArchive);
    archiveFile << "Test content";
    archiveFile.close();

    // Simulate receiving metadata
    std::string metadata = "test_archive.tar.gz|d41d8cd98f00b204e9800998ecf8427e";
    std::istringstream metadataStream(metadata);

    std::string filename, expectedMD5;
    std::getline(metadataStream, filename, '|');
    std::getline(metadataStream, expectedMD5);

    EXPECT_EQ(filename, "test_archive.tar.gz");
    EXPECT_EQ(expectedMD5, "d41d8cd98f00b204e9800998ecf8427e");

    // Clean up
    std::filesystem::remove(tempArchive);
}

TEST_F(FileReceiveTest, VerifyMD5) {
    // Create a temporary file to simulate the received archive
    std::string tempArchive = "test_archive.tar.gz";
    std::ofstream archiveFile(tempArchive);
    archiveFile << "Test content";
    archiveFile.close();

    // Simulate MD5 verification
    std::ifstream file(tempArchive, std::ios::binary);
    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    char buffer[65536];
    while (file.good()) {
        file.read(buffer, 65536);
        MD5_Update(&md5Context, buffer, file.gcount());
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);

    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    std::string actualMD5 = ss.str();
    std::string expectedMD5 = "d41d8cd98f00b204e9800998ecf8427e"; // Example MD5

    EXPECT_EQ(actualMD5, expectedMD5);

    // Clean up
    std::filesystem::remove(tempArchive);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class FileReceiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code, if needed
    }

    void TearDown() override {
        // Cleanup code, if needed
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
    archiveFile << ""; // Empty content to match the expected MD5 hash
    archiveFile.close();

    // Simulate MD5 verification
    std::string cmd = "md5sum " + tempArchive + " | awk '{print $1}'";
    FILE* pipe = popen(cmd.c_str(), "r");
    char buffer[33] = {0};
    fgets(buffer, sizeof(buffer), pipe);
    pclose(pipe);
    buffer[32] = '\0'; // Remove newline

    std::string actualMD5(buffer);
    std::string expectedMD5 = "d41d8cd98f00b204e9800998ecf8427e"; // Example MD5

    EXPECT_EQ(actualMD5, expectedMD5);

    // Clean up
    std::filesystem::remove(tempArchive);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 
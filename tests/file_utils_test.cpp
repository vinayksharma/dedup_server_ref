#include <gtest/gtest.h>
#include "core/file_utils.hpp"
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>

namespace fs = std::filesystem;

class FileUtilsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create test directory structure
        fs::create_directories("test_dir/subdir1");
        fs::create_directories("test_dir/subdir2");

        // Create test files
        std::ofstream("test_dir/file1.txt").close();
        std::ofstream("test_dir/file2.txt").close();
        std::ofstream("test_dir/subdir1/file3.txt").close();
        std::ofstream("test_dir/subdir2/file4.txt").close();
    }

    void TearDown() override
    {
        // Clean up test directory
        fs::remove_all("test_dir");
    }
};

TEST_F(FileUtilsTest, ListFilesNonRecursive)
{
    std::vector<std::string> files;
    bool completed = false;
    bool error_occurred = false;

    auto observable = FileUtils::listFilesAsObservable("test_dir", false);
    observable.subscribe(
        [&files](const std::string &file_path)
        {
            files.push_back(file_path);
        },
        [&error_occurred](const std::exception &e)
        {
            error_occurred = true;
            FAIL() << "Unexpected error in file listing: " << e.what();
        },
        [&completed]()
        {
            completed = true;
        });

    // Should find 2 files in the root directory
    EXPECT_EQ(files.size(), 2);
    EXPECT_TRUE(completed);
    EXPECT_FALSE(error_occurred);

    // Check that we have the expected files
    bool found_file1 = false, found_file2 = false;
    for (const auto &file : files)
    {
        if (file.find("file1.txt") != std::string::npos)
            found_file1 = true;
        if (file.find("file2.txt") != std::string::npos)
            found_file2 = true;
    }
    EXPECT_TRUE(found_file1);
    EXPECT_TRUE(found_file2);
}

TEST_F(FileUtilsTest, ListFilesRecursive)
{
    std::vector<std::string> files;
    bool completed = false;
    bool error_occurred = false;

    auto observable = FileUtils::listFilesAsObservable("test_dir", true);
    observable.subscribe(
        [&files](const std::string &file_path)
        {
            files.push_back(file_path);
        },
        [&error_occurred](const std::exception &e)
        {
            error_occurred = true;
            FAIL() << "Unexpected error in file listing: " << e.what();
        },
        [&completed]()
        {
            completed = true;
        });

    // Should find 4 files total (2 in root + 1 in subdir1 + 1 in subdir2)
    EXPECT_EQ(files.size(), 4);
    EXPECT_TRUE(completed);
    EXPECT_FALSE(error_occurred);

    // Check that we have all expected files
    bool found_file1 = false, found_file2 = false, found_file3 = false, found_file4 = false;
    for (const auto &file : files)
    {
        if (file.find("file1.txt") != std::string::npos)
            found_file1 = true;
        if (file.find("file2.txt") != std::string::npos)
            found_file2 = true;
        if (file.find("file3.txt") != std::string::npos)
            found_file3 = true;
        if (file.find("file4.txt") != std::string::npos)
            found_file4 = true;
    }
    EXPECT_TRUE(found_file1);
    EXPECT_TRUE(found_file2);
    EXPECT_TRUE(found_file3);
    EXPECT_TRUE(found_file4);
}

TEST_F(FileUtilsTest, InvalidDirectory)
{
    bool error_received = false;
    std::string error_message;

    auto observable = FileUtils::listFilesAsObservable("nonexistent_dir", false);
    observable.subscribe(
        [](const std::string &file_path)
        {
            FAIL() << "Should not receive any files for invalid directory";
        },
        [&error_received, &error_message](const std::exception &e)
        {
            error_received = true;
            error_message = e.what();
        },
        []()
        {
            FAIL() << "Should not complete successfully for invalid directory";
        });

    EXPECT_TRUE(error_received);
    EXPECT_TRUE(error_message.find("Invalid directory path") != std::string::npos);
}
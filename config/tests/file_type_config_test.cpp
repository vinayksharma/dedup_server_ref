#include "poco_config_adapter.hpp"
#include "poco_config_manager.hpp"
#include <iostream>
#include <cassert>
#include <filesystem>

void testFileTypeConfiguration()
{
    std::cout << "Testing file type configuration..." << std::endl;

    try
    {
        auto &config = PocoConfigAdapter::getInstance();

        // Test initial state
        auto supported_types = config.getSupportedFileTypes();
        auto transcoding_types = config.getTranscodingFileTypes();

        std::cout << "Initial supported types count: " << supported_types.size() << std::endl;
        std::cout << "Initial transcoding types count: " << transcoding_types.size() << std::endl;

        // Test enabling/disabling specific file types
        config.setFileTypeEnabled("images", "jpg", true);
        config.setFileTypeEnabled("images", "png", false);

        // Test transcoding file type configuration
        config.setTranscodingFileType("cr2", true);
        config.setTranscodingFileType("nef", false);

        // Verify changes
        auto updated_supported = config.getSupportedFileTypes();
        auto updated_transcoding = config.getTranscodingFileTypes();

        if (updated_supported.at("jpg"))
        {
            std::cout << "✅ JPG enabled successfully" << std::endl;
        }
        else
        {
            std::cout << "❌ JPG not enabled" << std::endl;
        }

        if (!updated_supported.at("png"))
        {
            std::cout << "✅ PNG disabled successfully" << std::endl;
        }
        else
        {
            std::cout << "❌ PNG not disabled" << std::endl;
        }

        std::cout << "✅ File type configuration test passed!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ File type configuration test failed: " << e.what() << std::endl;
        throw;
    }
}

int main()
{
    try
    {
        testFileTypeConfiguration();
        std::cout << "✅ All tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
#include "core/file_utils.hpp"
#include <stdexcept>
#include <iostream>

namespace fs = std::filesystem;

SimpleObservable<std::string> FileUtils::listFilesAsObservable(const std::string &dir_path, bool recursive)
{
    return listFilesInternal(dir_path, recursive);
}

SimpleObservable<std::string> FileUtils::listFilesInternal(const std::string &dir_path, bool recursive)
{
    return SimpleObservable<std::string>(
        [dir_path, recursive](auto onNext, auto onError, auto onComplete)
        {
            try
            {
                // Validate directory exists
                if (!isValidDirectory(dir_path))
                {
                    if (onError)
                    {
                        onError(std::runtime_error("Invalid directory path: " + dir_path));
                    }
                    return;
                }

                if (recursive)
                {
                    // Custom recursive directory iteration with error handling
                    scanDirectoryRecursively(dir_path, onNext);
                }
                else
                {
                    // Non-recursive directory iteration
                    for (const auto &entry : fs::directory_iterator(dir_path))
                    {
                        if (entry.is_regular_file())
                        {
                            onNext(entry.path().string());
                        }
                    }
                }

                if (onComplete)
                {
                    onComplete();
                }
            }
            catch (const std::exception &e)
            {
                if (onError)
                {
                    onError(e);
                }
            }
        });
}

bool FileUtils::isValidDirectory(const std::string &path)
{
    try
    {
        fs::path dir_path(path);
        return fs::exists(dir_path) && fs::is_directory(dir_path);
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

void FileUtils::scanDirectoryRecursively(const std::string &dir_path,
                                         std::function<void(const std::string &)> onNext)
{
    // Custom recursive scanner that handles permission errors gracefully
    std::function<void(const fs::path &)> scanDirectory = [&](const fs::path &current_path)
    {
        try
        {
            for (const auto &entry : fs::directory_iterator(current_path))
            {
                try
                {
                    if (entry.is_regular_file())
                    {
                        onNext(entry.path().string());
                    }
                    else if (entry.is_directory())
                    {
                        // Recursively scan subdirectories
                        scanDirectory(entry.path());
                    }
                }
                catch (const fs::filesystem_error &e)
                {
                    // Log the error but continue scanning
                    std::cerr << "Warning: Skipping entry due to permission error: " << entry.path().string()
                              << " - " << e.what() << std::endl;
                    continue;
                }
            }
        }
        catch (const fs::filesystem_error &e)
        {
            // Log the error but don't stop the entire scan
            std::cerr << "Warning: Error accessing directory " << current_path.string() << ": " << e.what() << std::endl;
        }
        catch (const std::exception &e)
        {
            // Log any other errors
            std::cerr << "Warning: Unexpected error scanning directory " << current_path.string() << ": " << e.what() << std::endl;
        }
    };

    // Start the recursive scan
    scanDirectory(fs::path(dir_path));
}
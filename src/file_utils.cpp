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
                    // Recursive directory iteration
                    for (const auto &entry : fs::recursive_directory_iterator(dir_path))
                    {
                        if (entry.is_regular_file())
                        {
                            onNext(entry.path().string());
                        }
                    }
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
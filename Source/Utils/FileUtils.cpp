//========= Copyright N11 Software, All rights reserved. ============//
//
// File: FileUtils.cpp
// Purpose: various file-related functions.
//
//===================================================================//

#include "FileUtils.h"

#include <filesystem>
#include <iostream>
#include <regex>

//-----------------------------------------------------------------------------
// recursively searches thru directories
//-----------------------------------------------------------------------------
void recursiveSearch(const std::filesystem::path& dir, std::vector<std::string>& paths) {
    if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (std::filesystem::is_regular_file(entry.status())) {
                paths.push_back(entry.path().string()); 
            }
        }
    }
}

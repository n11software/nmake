#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <filesystem>
#include <vector>
#include <string>

void recursiveSearch(const std::filesystem::path& dir, std::vector<std::string>& paths);

#endif /* FILEUTILS_H */

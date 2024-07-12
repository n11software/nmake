#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <vector>

std::string lterm(const std::string& str);
std::string rterm(const std::string& str);
std::string trim(const std::string& str);
bool isDigits(const std::string& str);
std::vector<std::string> split(const std::string& str, char delimiter);

#endif /* STRINGTUTILS_H */

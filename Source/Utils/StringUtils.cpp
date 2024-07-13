//========= Copyright N11 Software, All rights reserved. ============//
//
// File: StringUtils.cpp
// Purpose: various string-related functions
//
//===================================================================//

#include "StringUtils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

std::string ltrim(const std::string& str) {
	auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	});
}

std::string rtrim(const std::string& str) {
	auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	});
	return std::string(str.begin(), end.base());
}

//-----------------------------------------------------------------------------
// trims whitespace off of beginning and end of string
//-----------------------------------------------------------------------------
std::string trim(const std::string& str) {
	return ltrim(rtrim(str));
}

bool isDigits(const std::string& str) {
	for (char ch : str) {
		int v = ch;
		if (!(ch >= 48 && ch <= 57)) {
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// splits string using a specific character
//-----------------------------------------------------------------------------
std::vector<std::string> split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(str);
	while (std::getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

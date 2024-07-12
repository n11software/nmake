#include "Utils/FileUtils.h"
#include "Utils/StringUtils.h"
#include "Utils/TerminalUtils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string.h>
#include <regex>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <variant>
#include <filesystem>
#include <cstddef>

std::vector<std::string> findExecutables(const std::string& program) {
  std::vector<std::string> paths;
  const char* pathEnv = std::getenv("PATH");
  if (!pathEnv) {
    std::cerr << "PATH environment variable not found." << std::endl;
    return paths;
  }

  std::vector<std::string> directories = split(pathEnv, ':');

  for (const auto& dir : directories) {
    std::filesystem::path p = dir + '/' + program;
    if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p))
      paths.push_back(p.string());
  }
  return paths;
}

std::string getEnvVar(std::string key) {
  char* val = getenv( key.c_str() );
  return val == (void*)0 ? std::string("") : std::string(val);
}

std::string to_lower(const std::string& str) {
  std::string newstr = str;
  std::transform(newstr.begin(), newstr.end(), newstr.begin(),
    [](unsigned char c){ return std::tolower(c); });
  return newstr;
}

int main(int argc, char **argv) {
  bool newEnv = false, addingToProject = false, isCustom = false;
  std::vector<std::string> customCommand;
  std::string envName = "";
  unsigned char type = 0, lang;
  // Read arguments
  for (int x = 1; x < argc; x++) {
    switch (x) {
      case 1:
        if (!strcmp(argv[1], "new")) newEnv = true;
        else if (!strcmp(argv[1], "add")) addingToProject = true;
        else {
          isCustom = true;
          customCommand.push_back(argv[x]);
        }
        break;
      case 2:
        if (newEnv || addingToProject) {
          if (std::regex_match(argv[x], std::regex("[A-Za-z0-9]+"))) {
            envName = argv[x];
          } else {
            std::cerr << "Invalid environment name: " << argv[x] << std::endl;
            return 1;
          }
        }
        break;
      default:
        // Handle custom command arguments
        customCommand.push_back(argv[x]);
        break;
    } 
  }

  // Create new environment if needed.
  if (newEnv) {
    if (envName == "") {
      std::cout << "Environment name: ";
      std::string input;
      std::getline(std::cin, input);
      if (std::regex_match(input, std::regex("[A-Za-z0-9]+"))) {
        envName = input;
        moveCursorAndClear(1);
      } else {
        std::cerr << "Invalid environment name: " << input << std::endl;
        return 1;
      }
    }

    std::vector<std::string> types = {"Program", "Library"};
    std::cout << "Select the type of environment:\n" << std::endl;
    std::string t = optionMenu(types);

    std::vector<std::string> langs = {"C", "C++", "Assembly"};
    if (t == "Library") langs.pop_back();
    std::cout << "Select the template:\n" << std::endl;
    std::string l = optionMenu(langs);

    std::cout << "Generating config...";

    // Write config to file
    std::transform(t.begin(), t.end(), t.begin(),
      [](unsigned char c){ return std::tolower(c); });
    std::ofstream conf("config", std::ios::binary);
    conf << "Name = \"" << envName << "\"\n";
    conf << "Type = \"" << t << "\"\n";
    for (auto &arg : customCommand) {
      conf << arg << "\n";
    }
    conf.close();

    moveCursorAndClear(1);

    std::cout << "Creating files...";

    mkdir("Source", 0700);
    mkdir("Build", 0700);
    
    if (l == "C") {
      std::ofstream src("Source/" + envName + ".c");
      src << "#include <stdio.h>\n\n";
      src << "int main(int argc, char** argv) {\n";
      src << "  printf(\"Hello, World!\\n\");\n";
      src << "  return 0;\n}";
      src.close();
    } else if (l == "C++") {
      std::ofstream src("Source/" + envName + ".cpp");
      src << "#include <iostream>\n\n";
      src << "int main(int argc, char** argv) {\n";
      src << "  std::cout << \"Hello, World!\" << std::endl;\n";
      src << "  return 0;\n}";
      src.close();
    } else if (l == "Assembly") {
      std::ofstream src("Source/" + envName + ".asm");
      src.close();
    }

    moveCursorAndClear(1);

    return 0;
  }

  // Continue to parse config
  std::string configPath = "config";
  std::ifstream config = std::ifstream(configPath.c_str(), std::ios::binary);

  if (!config.is_open()) {
    std::cerr << "Failed to find a config file at: " << configPath << std::endl;
    return 1;
  }

  std::string c_comp = "", cpp_comp = "", asm_comp = "", ld = "g++";

  std::vector<std::pair<std::string, std::variant<int, bool, std::string>>> vars;

  std::string toks = "";
  int lenOfConfig = std::filesystem::file_size(configPath);
  bool inVar = false;
  bool inQuotes = false;
  bool inCommand = false;
  bool maybeACall = false;
  std::string callName = "";
  std::string func = "";
  std::map<std::string, std::vector<std::string>> funcs;
  char quoteType = 0;
  for (int c = 0; c < lenOfConfig+1; c++) {
    char ch = c==lenOfConfig?'\n':config.get();
    switch (ch) {
      case '=':
        inVar = true;
        toks+=ch;
        break;
      case '\n':
        if (inVar) {
          // split by =
          std::vector<std::string> parts;
          parts.push_back(trim(toks.substr(0, toks.find_first_of('='))));
          parts.push_back(trim(toks.substr(toks.find_first_of('=')+1)));
          if (parts.size()!= 2) {
            std::cerr << "Invalid config format: " << toks << std::endl;
            return 1;
          }

          if (parts[1][0] == '"' || parts[1][0] == '\'') {
            quoteType = parts[1][0];
            inQuotes = true;
          } else {
            if (isDigits(parts[1])) {
              vars.push_back({parts[0], stoi(parts[1])});
            } else {
              std::transform(parts[1].begin(), parts[1].end(), parts[1].begin(),
                [](unsigned char c){ return std::tolower(c); });
              if (parts[1] == "true") vars.push_back({parts[0], true});
              else if (parts[1] == "false") vars.push_back({parts[0], false});
            }
          }
          while (inQuotes) {
            if (parts[1][parts[1].size()-1] == quoteType) {
              inQuotes = false;
              parts[1] = parts[1].substr(1, parts[1].size()-2);
              vars.push_back({parts[0], parts[1]});
            } // TODO: Add \ for multi line
          }
          if (!std::regex_match(parts[0], std::regex("[A-Za-z0-9]+"))) {
            std::cerr << "Invalid variable name: " << parts[0] << std::endl;
            return 1;
          }
          inVar = false;
        } else if (inCommand) {
          std::string command = trim(toks);
          if (func == "") {
            system(command.c_str());
          } else {
            funcs[func].push_back(command);
          }
        } else if (maybeACall) {
          std::vector<std::string> commands = funcs[callName];
          for (auto &cmd : commands) {
            std::cout << cmd << std::endl;
            system(cmd.c_str());
          }
          maybeACall = false;
          callName = "";
        }
        toks = "";
        break;
      case '{': {
        std::string funName = callName.substr(0, toks.find('('));
        if (!std::regex_match(funName, std::regex("[A-Za-z0-9]+"))) {
          std::cerr << "Invalid function name: " << funName << std::endl;
          return 1;
        }
        callName = "";
        func = funName;
        funcs[funName] = std::vector<std::string>();
        maybeACall = false;
        toks = "";
        break;
      }
      case '}':
        func = "";
        toks = "";
        break;
      case '(':
        maybeACall = true;
        callName = toks.substr(0, toks.find('('));
        toks = "";
        break;
      case ')':
        break;
      default:
        toks+=ch;
        break;
    }
    if (trim(to_lower(toks)) == "run") {
      inCommand = true;
      toks = "";
    }
  }

  std::string SourceDir = "Source/", BuildDir = "Build/", OutPath;
  std::string CF, CXXF, ASF, LDF;

  for (auto &var : vars) {
    if (var.first == "Name") envName = std::get<std::string>(var.second);
    else if (var.first == "Type") type = std::get<std::string>(var.second) == "Program"? 1 : 2;
    else if (var.first == "CC") c_comp = std::get<std::string>(var.second);
    else if (var.first == "CXX") cpp_comp = std::get<std::string>(var.second);
    else if (var.first == "AS") asm_comp = std::get<std::string>(var.second);
    else if (var.first == "Source") SourceDir = std::get<std::string>(var.second);
    else if (var.first == "Build") BuildDir = std::get<std::string>(var.second);
    else if (var.first == "Output") OutPath = std::get<std::string>(var.second);
    else if (var.first == "CC_FLAGS") CF = std::get<std::string>(var.second);
    else if (var.first == "CXX_FLAGS") CXXF = std::get<std::string>(var.second);
    else if (var.first == "AS_FLAGS") ASF = std::get<std::string>(var.second);
    else if (var.first == "LD_FLAGS") LDF = std::get<std::string>(var.second);
    else if (var.first == "LD") ld = std::get<std::string>(var.second);
  }

  if (OutPath.empty()) OutPath = "./" + envName;

  if (c_comp == "") {
    if (!getEnvVar("CC").empty()) {
      c_comp = getEnvVar("CC");
    } else {
      std::vector<std::string> paths = findExecutables("gcc");
      if (paths.empty()) {
        paths = findExecutables("clang");
        if (paths.empty()) {
          std::cerr << "No C compiler found. Please declare CC." << std::endl;
          return 1;
        }
        c_comp = paths[0];
      } else {
        c_comp = paths[0];
      }
    }
  }

  if (cpp_comp == "") {
    if (!getEnvVar("CXX").empty()) {
      cpp_comp = getEnvVar("CXX");
    } else {
      std::vector<std::string> paths = findExecutables("g++");
      if (paths.empty()) {
        paths = findExecutables("clang++");
        if (paths.empty()) {
          std::cerr << "No C compiler found. Please declare CXX." << std::endl;
          return 1;
        }
        cpp_comp = paths[0];
      } else {
        cpp_comp = paths[0];
      }
    }
  }

  if (asm_comp == "") {
    if (!getEnvVar("AS").empty()) {
      asm_comp = getEnvVar("AS");
    } else {
      std::vector<std::string> paths = findExecutables("nasm");
      if (paths.empty()) {
        paths = findExecutables("as");
        if (paths.empty()) {
          std::cerr << "No C compiler found. Please declare AS." << std::endl;
          return 1;
        }
        asm_comp = paths[0];
      } else {
        asm_comp = paths[0];
      }
    }
  }

  std::vector<std::string> paths;
  recursiveSearch(SourceDir, paths);

  for (const auto& path: paths) {
    std::cout << "Compiling '" << path << "'" << std::endl;
    std::string compile_command, flags;
    int language;
    std::string ext = path.substr(path.find_last_of('.'));
    if (ext == ".cpp" || ext == ".c++") {
      compile_command = cpp_comp;
      flags = CF;
    } else if (ext == ".asm" || ext == ".S") {
      compile_command = asm_comp;
      flags = CXXF;
    } else if (ext == ".c") {
      compile_command = c_comp;
      flags = ASF;
    }

    std::string clean_out = path.substr(path.find_first_of(SourceDir)+SourceDir.size());

    compile_command += " -c " + path + " -o " + BuildDir + "/" + clean_out + ".o" + flags;

    system(compile_command.c_str());
    moveCursorAndClear(1);
  }

  std::string link_command = ld + " -o " + OutPath + " " + LDF;
  for (const auto& path: paths) {
    std::string clean_out = path.substr(path.find_first_of(SourceDir)+SourceDir.size());
    link_command += BuildDir + "/" + clean_out + ".o ";
  }

  std::cout << "Linking: " << link_command << std::endl;
  system(link_command.c_str());
  moveCursorAndClear(1);

  std::cout << "Build completed successfully!" << std::endl;

  return 0;
}

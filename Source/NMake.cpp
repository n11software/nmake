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

char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0) {
        perror("tcgetattr");
    }
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0) {
        perror("tcsetattr");
    }
    if (read(0, &buf, 1) < 0) {
        perror("read");
    }
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) {
        perror("tcsetattr");
    }
    return buf;
}

void setTextColor(int color) {
    if (color == 9) // Blue color for selected option
        std::cout << "\033[1;34m";
    else
        std::cout << "\033[0m"; // Reset color
}

void moveCursorUp(int lines) {
    std::cout << "\033[" << lines << "A";
}

void moveCursorAndClear(int lines) {
  moveCursorUp(lines);
  for (int i = 0; i < lines;i++) std::cout << "                                                                                                        \n";
  moveCursorUp(lines);
}

void clearScreen() {
    std::cout << "\033[H\033[J";
}

std::string optionMenu(const std::vector<std::string>& opt) {
  int selected = 0;
  for (int i = 0; i < opt.size(); ++i) {
    if (i == selected) {
      setTextColor(9); // Blue color
      std::cout << "> " << opt[i] << "\n";
      setTextColor(0); // Reset color
    } else std::cout << "  " << opt[i] << "\n";
  }
  while (true) {
    char ch = getch();
    if (ch == 27) { // Escape sequence
      ch = getch();
      if (ch == 91) { // Arrow keys
        ch = getch();
        if (ch == 65) { // Up arrow
          if (selected > 0) selected--;
          else selected = opt.size() - 1;
        } else if (ch == 66) { // Down arrow
          if (selected < opt.size() - 1) selected++;
          else selected = 0;
        }
      }
    } else if (ch == 10) break;

    moveCursorAndClear(opt.size());
    for (int i = 0; i < opt.size(); ++i) {
      if (i == selected) {
        setTextColor(9); // Blue color
        std::cout << "> " << opt[i] << "\n";
        setTextColor(0); // Reset color
      } else std::cout << "  " << opt[i] << "\n";
    }
  }
  moveCursorAndClear(opt.size()+2);
  return opt[selected];
}

// Trim functions to remove whitespaces from the beginning and end of a string
std::string ltrim(const std::string& s) {
    auto start = std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    return std::string(start, s.end());
}

std::string rtrim(const std::string& s) {
    auto end = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    return std::string(s.begin(), end.base());
}

std::string trim(const std::string& s) {
    return ltrim(rtrim(s));
}

bool is_digits(std::string& str) {
  for (char ch : str) {
    int v = ch;
    if (!(ch >= 48 && ch <= 57)) {
      return false;
    }
  }
  return true;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);
  while (std::getline(tokenStream, token, delimiter)) tokens.push_back(token);
  return tokens;
}

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

void recursive_search(const std::filesystem::path& dir, std::vector<std::string>& paths) {
  if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
      if (std::filesystem::is_regular_file(entry.status()))
        paths.push_back(entry.path().string());
    }
  }
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
            if (is_digits(parts[1])) {
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
  recursive_search(SourceDir, paths);

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
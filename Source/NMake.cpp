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
  for (int i = 0; i < lines;i++) std::cout << "                                               \n";
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
    conf << "Name = " << envName << "\n";
    conf << "Type = " << t << "\n";
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
}
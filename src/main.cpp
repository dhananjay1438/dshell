#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

class InBuiltCommands {
  // TODO Implement these methods and try to learn about these
public:
  void cd();
  void mv();
  void mkdir();
  void rm();
};

std::string dshell_read_line() {
  std::string line;
  std::getline(std::cin, line);
  return line;
}

std::vector<std::string> dshell_split_line(std::string str) {

  std::vector<std::string> tokens;
  std::string::size_type whitespace, current = 0;

  while ((whitespace = str.find(" ", current)) != std::string::npos) {
    tokens.push_back(str.substr(current, whitespace - current));
    current = whitespace + 1;
  }
  tokens.push_back(str.substr(current, whitespace - current));

  return tokens;
}

int dshell_execute(std::vector<std::string> args) {

  pid_t pid, wpid;

  int status;
  // Why the hell char* argv[] was causing error and not this one
  char **argv = (char **)malloc(sizeof(char *) * args.size());

  // converting vector to char** (required for execvp)
  for (size_t i = 0; i < args.size(); i++) {
    argv[i] = const_cast<char *>(args[i].c_str());
  }
  pid = fork();
  if (pid == 0) {
    char buffer[1024];
    getcwd(buffer, 1024);
    char *current_directory = buffer;
    if ((strcmp(argv[0], "cd") == 0)) {
      if (argv[1][0] == '/') {
        chdir(argv[1]);
      } else if (argv[1][0] == '.') {
        chdir(argv[1]);
      } else if (argv[1] != nullptr) {
        strcat(current_directory, "/");
        strcat(current_directory, argv[1]);
        chdir(current_directory);
      } else {
        chdir(getenv("HOME"));
      }
    } else if (strcmp(argv[0], "ls") == 0) {
      execvp("ls", argv);
    } else {
      execvp(argv[0], argv);
    }
  } else {
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

void dshell_loop() {

  std::string line;
  std::vector<std::string> args;
  int status = 0;
  do {
    std::cout << "~ ";
    line = dshell_read_line();
    args = dshell_split_line(line);
    status = dshell_execute(args);
  } while (status != 0);
}

int main(int argc, char **argv) {

  dshell_loop();
  return 0;
}

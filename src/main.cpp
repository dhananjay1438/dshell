#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// TODO: Make cat command work with pipes
// TODO: input cat main.cpp | wc then press Ctrl+c to close the running command,
// then press enter it wil just give some error occured and it creates two
// processes
#define SHELL_NAME "dshell"
std::fstream DSHELL_HISTORY_FILE(".dshell_history", std::ios::app);

std::string get_current_directory();
void execute_pipe_commands(std::string line);
void dshell_loop();
class builtInCommands {
  // TODO Implement these methods and try to learn about these
public:
  void cd();
  void mv();
  void mkdir();
  void rm();
};
// Added history feature

// TODO: Implement "mv" command
// TODO: Implement "cp" command
// TODO: Add redirection operations
// TODO: Add pipe operator

class history {
private:
  int index;

public:
  std::vector<std::pair<int, std::string>> list;
  void add(std::string);
  void show(int);
  void show();
  history();
};
history::history() { index = 1; }
void history::add(std::string string) {
  list.push_back({index, string});
  index++;
}
void history::show(int num) {
  if (num <= 0) {
    std::cerr << "Enter number above 1\n";
    return;
  }
  for (size_t i = 0; i < static_cast<size_t>(num); i++) {
    std::cout << list[i].first << " " << list[i].second << "\n";
  }
}
void history::show() {
  for (auto &it : list) {
    std::cout << it.first << " " << it.second << "\n";
  }
}

history hist;
std::string trim(std::string command) {
  command.erase(command.find_last_not_of(' ') + 1);
  command.erase(0, command.find_first_not_of(' '));
  return command;
}

std::string dshell_read_line() {
  std::string line;
  std::getline(std::cin, line);
  return line;
}

std::vector<std::string> split_line_with_delimiter(std::string line,
                                                   char split) {

  // If pipe is used
  std::vector<std::string> tokens;
  std::string::size_type whitespace, current = 0;

  while ((whitespace = line.find(split, current)) != std::string::npos) {
    tokens.push_back(trim(line.substr(current, whitespace - current)));
    current = whitespace + 1;
  }
  tokens.push_back(trim(line.substr(current, whitespace - current)));

  return tokens;
}

bool check_if_command_exists(char *command) {
  std::string path = getenv("PATH");
  std::vector<std::string> paths = split_line_with_delimiter(path, ':');
  struct stat stats;
  // If command is found at any location from path returns true
  for (auto &loc : paths) {
    loc = loc.append("/");
    if (stat(loc.append(command).c_str(), &stats) == 0) {
      return true;
    }
  }
  return false;
}
void change_directory(char *current_directory, char **argv, size_t size) {
  if (size == 1) {
    chdir(getenv("HOME"));
  } else if (argv[1][0] == '/') {
    chdir(argv[1]);
  } else if (argv[1][0] == '.') {
    chdir(argv[1]);
  } else if (argv[1][0] == '~') {
    chdir(getenv("HOME"));
  } else if (argv[1] != nullptr) {
    strcat(current_directory, argv[1]);
    chdir(current_directory);
  }
}
void get_history(char **argv, size_t size) {
  if (size == 2) {
    hist.show(std::stoi(argv[1]));
  } else {
    hist.show();
  }
}
void copy_file(char **argv, size_t size, bool is_to_be_deleted) {
  // Working on cp command
  if (size != 3) {
    std::cerr << "Error: Can't copy!\nUsage: cp source destination";
    return;
  }
  // Can copy from absolute path only

  std::fstream read(argv[1], std::ios::in);
  std::fstream write(argv[2], std::ios::out);
  std::stringstream sstream;

  if (read.fail()) {
    std::cerr << "No such file or directory " << argv[1];
    return;
  }
  if (write.fail()) {
    std::cerr << "No such file or directory " << argv[2];
    return;
  }
  sstream << read.rdbuf();
  write.write(sstream.str().c_str(), static_cast<long>(sstream.str().length()));
  sstream.clear();
  if (is_to_be_deleted) {
    std::remove(argv[1]);
  }
  read.close();
  write.close();
}
void cat(char **argv, size_t size) {

  if (size == 1) {
    std::cerr << "Error: Operand required\nUsage:cp filename\n";
    return;
  }
  std::fstream file(argv[1], std::ios::in);

  if (file.is_open()) {
    std::cout << file.rdbuf();
    file.close();
  } else {
    std::cerr << "Error: No such file exists\n";
    return;
  }
}
char **convert_vector_to_char_star_star(std::vector<std::string> args) {

  char **argv = (char **)malloc(sizeof(char *) * args.size());

  // converting vector to char** (required for execvp)
  for (size_t i = 0; i < args.size(); i++) {
    argv[i] = const_cast<char *>(args[i].c_str());
  }
  return argv;
}
void sig_int_handler(int _) {
  (void)_;
  std::string directory = get_current_directory();
  std::cout << "\n" << directory;
  std::fflush(stdout);
};

// TODO: Add one more bool for redirection
int dshell_execute(std::string line, bool uses_pipe) {

  pid_t pid, wpid;
  int status;

  // Why the hell char* argv[] was causing error and not this one

  std::vector<std::string> args = split_line_with_delimiter(line, ' ');
  char **argv = (char **)malloc(sizeof(char *) * args.size());

  // converting vector to char** (required for execvp)
  for (size_t i = 0; i < args.size(); i++) {
    argv[i] = const_cast<char *>(args[i].c_str());
  }
  /* Required inorder to change directory suppose you are at /path (i.e
   current directory )and if you didn't append "/" at the end then the path
   becomes /pathfile instead of path/file */
  char *current_directory = get_current_dir_name();
  strcat(current_directory, "/");

  /*
   * Not using this condition after forking because for every cd invokation it
   * creates a new process, For ex: If you do "cd .." and "cd .." it will create
   * two new processes (so in total 3)
   */
  if (!uses_pipe) {
    if ((strcmp(argv[0], "cd") == 0)) {
      change_directory(current_directory, argv, args.size());
      return 1;
    }
  }
  pid = fork();
  if (pid == 0) {
    if (uses_pipe) {
      execute_pipe_commands(line);
      return 1;
    }
    if ((strcmp(argv[0], "history")) == 0) {
      get_history(argv, args.size());
    } else if ((strcmp(argv[0], "cd") == 0)) {
      change_directory(current_directory, argv, args.size());
    } else if (!check_if_command_exists(argv[0])) {
      std::cerr << "dshell: command not found: "
                << "'" << argv[0] << "'\n";
      return -1;
    } else if (strcmp(argv[0], "cp") == 0) {
      copy_file(argv, args.size(), false);
    } else if (strcmp(argv[0], "cat") == 0) {
      cat(argv, args.size());
    } else if (strcmp(argv[0], "mv") == 0) {
      copy_file(argv, args.size(), true);
    } else {
      if (execvp(argv[0], argv) < 0) {
        std::cerr << "Some error occured";
      }
    }
  } else {
    wpid = waitpid(pid, &status, WUNTRACED);
    free(argv);
  }
  return 1;
}
void execute(int in, int out, std::vector<std::string> args) {

  char **argv = (char **)malloc(sizeof(char *) * args.size());

  // converting vector to char** (required for execvp)
  for (size_t i = 0; i < args.size(); i++) {
    argv[i] = const_cast<char *>(args[i].c_str());
  }
  pid_t pid = fork();
  if (pid == 0) {
    if (in != 0) {
      dup2(in, 0);
      close(in);
    }
    if (out != 1) {
      dup2(out, 1);
      close(out);
    }
    if (strcmp(argv[0], "cat") == 0) {
      cat(argv, args.size());
    } else {
      execvp(argv[0], argv);
    }
  } else {
    wait(NULL);
  }
}
void execute_pipe_commands(std::string line) {
  std::vector<std::string> commands = split_line_with_delimiter(line, '|');
  int fd[2];
  int in = 0;
  size_t i;
  for (auto string : commands) {
    std::vector<std::string> command = split_line_with_delimiter(string, ' ');
    if (!check_if_command_exists(const_cast<char *>(command[0].c_str()))) {
      std::cerr << SHELL_NAME << ": command not found '" << command[0] << "'"
                << std::endl;
    }
  }
  for (i = 0; i < commands.size() - 1; i++) {
    pipe(fd);
    execute(in, fd[1], split_line_with_delimiter(trim(commands[i]), ' '));
    close(fd[1]);
    in = fd[0];
  }
  if (in != 0) {
    dup2(in, 0);
  }
  trim(commands[i]);
  std::vector<std::string> args = split_line_with_delimiter(commands[i], ' ');
  char **argv = (char **)malloc(sizeof(char *) * args.size());

  // converting vector to char** (required for execvp)
  for (size_t index = 0; index < args[index].size(); index++) {
    argv[index] = const_cast<char *>(args[index].c_str());
  }
  execvp(argv[0], argv);
}

std::string get_current_directory() {
  char buff[1024];
  getcwd(buff, 1024);
  std::string current_directory(buff);
  std::string to_remove = "/home/dhananjay";
  size_t loc = current_directory.find(to_remove);
  if (loc < current_directory.length()) {
    current_directory.replace(loc, to_remove.size(), "~");
  }
  current_directory.append(" $ ");
  return current_directory;
}
void add_to_history(std::string line) {
  if (DSHELL_HISTORY_FILE.bad()) {
    std::cerr << "Cannot create history file for dshell, you will only be able "
                 "to see history from this session\n";
  }
  /* If history command itself is not found in command then only add it to
   *  history
   */
  if (line.find("history") == std::string::npos) {
    hist.add(line);
    DSHELL_HISTORY_FILE.write(line.c_str(), static_cast<long>(line.size()));
    // Adding newline after every line
    DSHELL_HISTORY_FILE.write("\n", 1);
  }
}
bool check_for_pipe(std::string line) {
  if (line.find('|') != std::string::npos) {
    return true;
  } else {
    return false;
  }
}

int main(void) {

  std::string line;
  std::vector<std::string> args;
  int status = 0;
  do {
    struct sigaction sg;
    sg.sa_flags = SA_RESTART;
    sg.sa_handler = sig_int_handler;
    sigaction(SIGINT, &sg, NULL);
    std::cout << get_current_directory();
    line = dshell_read_line();
    add_to_history(line);
    if (line.compare("exit") == 0 || line.compare("quit") == 0) {
      status = 0;
      return -1;
    }
    if (line.empty()) {
      status = 1;
      continue;
    }
    if (check_for_pipe(line)) {
      status = dshell_execute(line, true);
    } else {
      status = dshell_execute(line, false);
    }
  } while (status != 0);
  return 0;
}

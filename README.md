# Simple Shell

A Unix-style interactive shell implemented in C for Linux. The project demonstrates process creation, program execution, pipes, background jobs, built-in commands, command history, and process lifecycle management.

## Highlights

- Creates child processes with `fork()`
- Executes programs with `execvp()`
- Supports foreground and background commands
- Implements pipelines using `pipe()` and `dup2()`
- Includes `cd`, `history`, and `exit` built-ins
- Tracks child processes and command execution time
- Handles child completion with `waitpid()`

## Build

```bash
make
```

## Run

```bash
./shell
```

## Example

```text
ls -la
pwd
cd ..
cat file.txt | grep keyword
sleep 5 &
history
exit
```

## Architecture

The shell reads a command line, parses it into commands and arguments, identifies pipelines and background execution, creates the required child processes, connects pipeline file descriptors, and launches programs with `execvp()`. Foreground jobs are synchronized with `waitpid()`, while background jobs are tracked without blocking the prompt.

## OS concepts demonstrated

- Process creation and termination
- `fork()` / `exec()` process model
- Parent-child synchronization
- Pipes and file-descriptor redirection
- Background process management
- Shell built-ins
- Command history
- Process lifecycle handling

## Project structure

```text
simple-shell/
├── README.md
├── Makefile
├── .gitignore
└── shell.c
```

## Author

Dhruv Dagar

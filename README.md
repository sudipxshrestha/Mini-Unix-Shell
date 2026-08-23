# Mini UNIX Command Implementation

# C++ Unix Shell Implementation

A modular, lightweight Unix shell implementation in C++17 based on classic shell architectures. This shell parses user commands, handles process execution via system calls (`fork`, `execvp`), and supports pipes, I/O redirection, command chaining, background execution, and built-in commands.

---

## Features

* **Command Execution:** Runs standard binary commands using `execvp`.
* **I/O Redirection:** Supports input redirection (`<`), output redirection (`>`), and output append (`>>`).
* **Pipelining:** Inter-process communication using Unix pipes (`|`).
* **Command Sequences:** Sequential execution of commands separated by semicolons (`;`).
* **Background Jobs:** Asynchronous background execution using `&`.
* **Built-in Commands:**
  * `cd [path]` — Changes current working directory (defaults to `HOME` if no path is given).

---

## Project Structure

```text
Shell/
├── include/           # Header files
│   ├── cmd.hpp        # Data structures for command nodes & AST cleanup
│   ├── exec.hpp       # Execution engine and fork/panic utilities
│   └── parse.hpp      # Command parsing and string tokenization
├── src/               # Source files
│   ├── cmd.cpp        # Command object constructors & free logic
│   ├── exec.cpp       # Process management & system call wrappers
│   ├── parse.cpp      # Recursive descent parser & tokenizer
│   └── main.cpp       # Shell REPL entry point
├── Makefile           # Automated build script using GNU Make
└── README.md          # Project documentation

Prerequisites
Operating System: Linux / POSIX-compliant environment

Compiler: g++ or clang++ with C++17 support

Build Tools: make (or cmake v3.10+)

Building and Running
Method 1: Using GNU Make (Recommended)
To compile the project using the included Makefile


# Build the shell binary
make

# Run the shell
./shell

# Clean up build artifacts
make clean


Example Usage
Once the shell is running ($  prompt), you can execute commands like standard Unix shells:


# Basic command execution
$ ls -la

# Built-in cd command
$ cd /var/log

# Output redirection
$ echo "Hello, World!" > output.txt
$ cat < output.txt
$ echo "Appended line" >> output.txt

# Pipeline
$ cat /etc/passwd | grep root

# Background execution
$ sleep 5 &

# Command chaining
$ echo "First"; echo "Second"


Architecture Overview
The shell operates on a Read-Eval-Print Loop (REPL) architecture:

Tokenizer (gettoken): Breaks input strings into commands, arguments, and control symbols (|, >, <, ;, &).

Parser (parsecmd): Builds an Abstract Syntax Tree (AST) representing command hierarchies (e.g., pipecmd, redircmd, execcmd).

Execution (runcmd): Traverses the AST recursively, using POSIX primitives (fork, execvp, pipe, dup2, open, wait) to establish standard file descriptors and manage process control.

Memory Cleanup (freecmd): Recursively frees dynamically allocated AST nodes after execution finishes.

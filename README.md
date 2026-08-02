# Mini UNIX Command Implementation

# 🐚 C++ Shell

A lightweight, high-performance UNIX shell written in modern C++17. Inspired by the classic MIT xv6 operating system shell, this project modernizes the underlying architecture using **RAII smart pointers**, **OOP polymorphism**, and **standard library containers**.

---

## ✨ Features

- **Built-in Navigation:** Integrated `cd` support with `HOME` directory defaulting.
- **Process Management:** Executes external system binaries using POSIX system calls (`fork`, `execvp`).
- **I/O Redirection:** Full support for standard input (`<`), output overwrite (`>`), and output append (`>>`).
- **Command Pipelines:** Multistage process piping via anonymous UNIX pipes (`|`).
- **Command Chaining:** Sequential command execution separated by semicolons (`;`).
- **Background Execution:** Asynchronous job execution (`&`).
- **Modern C++ Memory Safety:** Managed using `std::unique_ptr` and dynamic vectors, preventing fixed argument limits (`MAXARGS`) and memory leaks.

---

## 🛠️ Requirements

- **Operating System:** Linux, macOS, or WSL (Windows Subsystem for Linux).
- **Compiler:** `g++` or `clang++` with C++17 support.
- **Build Tools:** Standard C/POSIX development libraries (`make`, `glibc`).

---

## 🚀 Quick Start

### 1. Build the Shell
Clone the repository and compile using `g++`:



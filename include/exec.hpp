#ifndef EXEC_HPP
#define EXEC_HPP

#include "cmd.hpp"

[[noreturn]] void runcmd(cmd *c);
[[noreturn]] void panic(const char *s);
int fork1();

#endif // EXEC_HPP
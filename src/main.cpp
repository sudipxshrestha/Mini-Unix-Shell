#include "cmd.hpp"
#include "exec.hpp"
#include "parse.hpp"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    static char buf[100];

    while (true) {
        std::cout << "$ " << std::flush;
        if (!std::cin.getline(buf, sizeof(buf))) break;

        // Built-in cd command
        if (buf[0] == 'c' && buf[1] == 'd' && (buf[2] == ' ' || buf[2] == '\0')) {
            buf[std::strlen(buf)] = 0;
            char *path = buf + 2;
            while (*path == ' ') path++;
            if (*path == 0) path = std::getenv("HOME");
            if (chdir(path) < 0) std::cerr << "cannot cd " << path << "\n";
            continue;
        }

        cmd *c = parsecmd(buf);
        if (fork1() == 0) {
            runcmd(c);
        }
        wait(nullptr);
        freecmd(c);
    }
    return 0;
}
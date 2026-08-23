#include "exec.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

void panic(const char *s) {
    std::cerr << s << "\n";
    std::exit(1);
}

int fork1() {
    int pid = fork();
    if (pid == -1) panic("fork");
    return pid;
}

void runcmd(cmd *c) {
    int p[2];

    if (c == nullptr) std::exit(0);

    switch (c->type) {
    default:
        panic("runcmd");

    case EXEC: {
        auto *ecmd = static_cast<execcmd*>(c);
        if (ecmd->argv[0] == nullptr) std::exit(0);
        execvp(ecmd->argv[0], ecmd->argv);
        std::cerr << "exec " << ecmd->argv[0] << " failed\n";
        break;
    }

    case REDIR: {
        auto *rcmd = static_cast<redircmd*>(c);
        close(rcmd->fd);
        if (open(rcmd->file, rcmd->mode, 0644) < 0) {
            std::cerr << "open " << rcmd->file << " failed\n";
            std::exit(1);
        }
        runcmd(rcmd->subcmd);
        break;
    }

    case LIST: {
        auto *lcmd = static_cast<listcmd*>(c);
        if (fork1() == 0)
            runcmd(lcmd->left);
        wait(nullptr);
        runcmd(lcmd->right);
        break;
    }

    case PIPE: {
        auto *pcmd = static_cast<pipecmd*>(c);
        if (pipe(p) < 0)
            panic("pipe");
        if (fork1() == 0) {
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->left);
        }
        if (fork1() == 0) {
            close(0);
            dup(p[0]);
            close(p[0]);
            close(p[1]);
            runcmd(pcmd->right);
        }
        close(p[0]);
        close(p[1]);
        wait(nullptr);
        wait(nullptr);
        break;
    }

    case BACK: {
        auto *bcmd = static_cast<backcmd*>(c);
        if (fork1() == 0)
            runcmd(bcmd->subcmd);
        break;
    }
    }
    std::exit(0);
}
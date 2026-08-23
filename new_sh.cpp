#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

// Command Base Structure
struct cmd {
    int type;
    virtual ~cmd() = default;
};

// Command Structures
struct execcmd : public cmd {
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
    execcmd() { 
        type = EXEC; 
        std::memset(argv, 0, sizeof(argv)); 
        std::memset(eargv, 0, sizeof(eargv)); 
    }
};

struct redircmd : public cmd {
    cmd *subcmd;
    char *file;
    char *efile;
    int mode;
    int fd;
    redircmd(cmd *sub, char *f, char *ef, int m, int target_fd)
        : subcmd(sub), file(f), efile(ef), mode(m), fd(target_fd) { type = REDIR; }
};

struct pipecmd : public cmd {
    cmd *left;
    cmd *right;
    pipecmd(cmd *l, cmd *r) : left(l), right(r) { type = PIPE; }
};

struct listcmd : public cmd {
    cmd *left;
    cmd *right;
    listcmd(cmd *l, cmd *r) : left(l), right(r) { type = LIST; }
};

struct backcmd : public cmd {
    cmd *subcmd;
    explicit backcmd(cmd *sub) : subcmd(sub) { type = BACK; }
};

// Helpers
int fork1();
void panic(const char*);
cmd *parsecmd(char*);

// Constructors
cmd* make_execcmd() {
    return new execcmd();
}

cmd* make_redircmd(cmd *subcmd, char *file, char *efile, int mode, int fd) {
    return new redircmd(subcmd, file, efile, mode, fd);
}

cmd* make_pipecmd(cmd *left, cmd *right) {
    return new pipecmd(left, right);
}

cmd* make_listcmd(cmd *left, cmd *right) {
    return new listcmd(left, right);
}

cmd* make_backcmd(cmd *subcmd) {
    return new backcmd(subcmd);
}

// Memory Cleanup
void freecmd(cmd *c) {
    if (c == nullptr) return;

    switch (c->type) {
    case REDIR: {
        auto *rcmd = static_cast<redircmd*>(c);
        freecmd(rcmd->subcmd);
        break;
    }
    case PIPE: {
        auto *pcmd = static_cast<pipecmd*>(c);
        freecmd(pcmd->left);
        freecmd(pcmd->right);
        break;
    }
    case LIST: {
        auto *lcmd = static_cast<listcmd*>(c);
        freecmd(lcmd->left);
        freecmd(lcmd->right);
        break;
    }
    case BACK: {
        auto *bcmd = static_cast<backcmd*>(c);
        freecmd(bcmd->subcmd);
        break;
    }
    }
    delete c;
}

// Execution Engine
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

// Parsing Utilities
const char whitespace[] = " \t\r\n\v";
const char symbols[] = "<|>&;";

int gettoken(char **ps, char *es, char **q, char **eq) {
    char *s = *ps;
    int ret;

    while (s < es && std::strchr(whitespace, *s)) s++;
    if (q) *q = s;
    ret = *s;
    switch (*s) {
    case 0:
        break;
    case '|': case ';': case '&': case '<':
        s++;
        break;
    case '>':
        s++;
        if (*s == '>') {
            ret = '+';
            s++;
        }
        break;
    default:
        ret = 'a';
        while (s < es && !std::strchr(whitespace, *s) && !std::strchr(symbols, *s)) s++;
        break;
    }
    if (eq) *eq = s;
    while (s < es && std::strchr(whitespace, *s)) s++;
    *ps = s;
    return ret;
}

int peek(char **ps, char *es, const char *toks) {
    char *s = *ps;
    while (s < es && std::strchr(whitespace, *s)) s++;
    *ps = s;
    return *s && std::strchr(toks, *s);
}

// Forward Declarations
cmd *parseline(char**, char*);
cmd *parsepipe(char**, char*);
cmd *parseexec(char**, char*);
cmd *nulterminate(cmd*);

cmd* parsecmd(char *s) {
    char *es = s + std::strlen(s);
    cmd *c = parseline(&s, es);
    peek(&s, es, "");
    if (s != es) panic("syntax error");
    return nulterminate(c);
}

cmd* parseline(char **ps, char *es) {
    cmd *c = parsepipe(ps, es);
    while (peek(ps, es, "&")) {
        gettoken(ps, es, nullptr, nullptr);
        c = make_backcmd(c);
    }
    if (peek(ps, es, ";")) {
        gettoken(ps, es, nullptr, nullptr);
        c = make_listcmd(c, parseline(ps, es));
    }
    return c;
}

cmd* parsepipe(char **ps, char *es) {
    cmd *c = parseexec(ps, es);
    if (peek(ps, es, "|")) {
        gettoken(ps, es, nullptr, nullptr);
        c = make_pipecmd(c, parsepipe(ps, es));
    }
    return c;
}

cmd* parseredirs(cmd *c, char **ps, char *es) {
    int tok;
    char *q, *eq;

    while (peek(ps, es, "<>")) {
        tok = gettoken(ps, es, nullptr, nullptr);
        if (gettoken(ps, es, &q, &eq) != 'a') panic("missing file for redirection");
        switch (tok) {
        case '<':
            c = make_redircmd(c, q, eq, O_RDONLY, 0);
            break;
        case '>':
            c = make_redircmd(c, q, eq, O_WRONLY | O_CREAT | O_TRUNC, 1);
            break;
        case '+': // >>
            c = make_redircmd(c, q, eq, O_WRONLY | O_CREAT | O_APPEND, 1);
            break;
        }
    }
    return c;
}

cmd* parseexec(char **ps, char *es) {
    char *q, *eq;
    int tok, argc = 0;
    cmd *ret = make_execcmd();
    auto *ecmd = static_cast<execcmd*>(ret);

    ret = parseredirs(ret, ps, es);
    while (!peek(ps, es, "|&;")) {
        if ((tok = gettoken(ps, es, &q, &eq)) == 0) break;
        if (tok != 'a') panic("syntax");
        ecmd->argv[argc] = q;
        ecmd->eargv[argc] = eq;
        argc++;
        if (argc >= MAXARGS) panic("too many args");
        ret = parseredirs(ret, ps, es);
    }
    ecmd->argv[argc] = nullptr;
    ecmd->eargv[argc] = nullptr;
    return ret;
}

cmd* nulterminate(cmd *c) {
    if (c == nullptr) return nullptr;

    switch (c->type) {
    case EXEC: {
        auto *ecmd = static_cast<execcmd*>(c);
        for (int i = 0; ecmd->argv[i]; i++) *ecmd->eargv[i] = 0;
        break;
    }
    case REDIR: {
        auto *rcmd = static_cast<redircmd*>(c);
        nulterminate(rcmd->subcmd);
        *rcmd->efile = 0;
        break;
    }
    case PIPE: {
        auto *pcmd = static_cast<pipecmd*>(c);
        nulterminate(pcmd->left);
        nulterminate(pcmd->right);
        break;
    }
    case LIST: {
        auto *lcmd = static_cast<listcmd*>(c);
        nulterminate(lcmd->left);
        nulterminate(lcmd->right);
        break;
    }
    case BACK: {
        auto *bcmd = static_cast<backcmd*>(c);
        nulterminate(bcmd->subcmd);
        break;
    }
    }
    return c;
}

void panic(const char *s) {
    std::cerr << s << "\n";
    std::exit(1);
}

int fork1() {
    int pid = fork();
    if (pid == -1) panic("fork");
    return pid;
}

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

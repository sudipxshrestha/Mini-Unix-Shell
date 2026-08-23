#ifndef CMD_HPP
#define CMD_HPP

#include <cstddef>
#include <cstring>

constexpr int EXEC = 1;
constexpr int REDIR = 2;
constexpr int PIPE = 3;
constexpr int LIST = 4;
constexpr int BACK = 5;

constexpr int MAXARGS = 10;

struct cmd {
    int type;
    virtual ~cmd() = default;
};

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

cmd* make_execcmd();
cmd* make_redircmd(cmd *subcmd, char *file, char *efile, int mode, int fd);
cmd* make_pipecmd(cmd *left, cmd *right);
cmd* make_listcmd(cmd *left, cmd *right);
cmd* make_backcmd(cmd *subcmd);

void freecmd(cmd *c);

#endif // CMD_HPP
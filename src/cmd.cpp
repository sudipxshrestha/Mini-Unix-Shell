#include "cmd.hpp"

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
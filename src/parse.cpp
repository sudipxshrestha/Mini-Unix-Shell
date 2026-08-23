#include "parse.hpp"
#include "exec.hpp"
#include <cstring>
#include <fcntl.h>

namespace {
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

cmd *parseline(char**, char*);
cmd *parsepipe(char**, char*);
cmd *parseexec(char**, char*);
cmd *nulterminate(cmd*);

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
} // anonymous namespace

cmd* parsecmd(char *s) {
    char *es = s + std::strlen(s);
    cmd *c = parseline(&s, es);
    peek(&s, es, "");
    if (s != es) panic("syntax error");
    return nulterminate(c);
}
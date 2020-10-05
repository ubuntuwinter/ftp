#include "common.h"

// 命令列表映射
const char *cmdName[] = {
    "USER", "PASS", "RETR", "STOR", "QUIT", "SYST",
    "TYPE", "PORT", "PASV", "MKD", "CWD", "PWD", "LIST",
    "RMD", "RNFR", "RNTO"};

// 欢迎信息
const char *welcome = "Anonymous FTP server ready.\r\n";

// 获取命令对应的序号
int getIndexInCmdList(char *cmd)
{
    for (int i = 0; i < CMDLISTSIZE; i++)
    {
        if (strcmp(cmd, cmdName[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

// 解析命令
int parseCmd(Command *cmd, char *cmdString)
{
    return sscanf(cmdString, "%s %s", cmd->command, cmd->arg);
}

// 发送字符串到socket
int writeSentence(int connfd, char *buffer, int len)
{
    int p = 0;
    while (p < len)
    {
        int n = write(connfd, buffer + p, len - p);
        if (n < 0)
        {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return -1;
        }
        else
        {
            p += n;
        }
    }
    return p;
}

// 发送特定字符串
int writeCertainSentence(int connfd, char *buffer, char *str)
{
    strcpy(buffer, str);
    int len = strlen(buffer);
    return writeSentence(connfd, buffer, len);
}

// 从socket接收字符串
int readSentence(int connfd, char *buffer)
{
    int p = 0;
    while (1)
    {
        int n = read(connfd, buffer + p, MAXBUF - p);
        if (n < 0)
        {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            return -1;
        }
        else if (n == 0)
        {
            break;
        }
        else
        {
            p += n;
            if (buffer[p - 1] == '\n')
            {
                break;
            }
        }
    }
    if (buffer[p - 2] == '\r')
    {
        buffer[p - 2] = '\0';
        return p - 2;
    }
    else
    {
        buffer[p - 1] = '\0';
        return p - 1;
    }
}

// 返回相应操作
int response(Command *cmd, State *state, char *buffer)
{
    int code = 0;
    switch (getIndexInCmdList(cmd->command))
    {
    case USER:
        code = ftpUSER(cmd, state, buffer);
        break;
    case PASS:
        code = ftpPASS(cmd, state, buffer);
        break;
    default:
        if (writeCertainSentence(state->connection, buffer, "500 Invaild command.\r\n") < 0)
        {
            code = -1;
        }
        else
        {
            code = 0;
        }
        break;
    }
    return code;
}

// 发送欢迎信息
int ftpWelcome(State *state, char *buffer)
{
    strcpy(buffer, "220 ");
    strcat(buffer, welcome);
    int len = strlen(buffer);
    // 发送字符串到socket
    if (writeSentence(state->connection, buffer, len) < 0)
    {
        return -1;
    }
    return 0;
}

// 登陆
int ftpUSER(Command *cmd, State *state, char *buffer)
{
    // 判断是否登陆
    if (state->logged_in)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "500 You'v already logged in.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 判断是否已经输入用户名
    if (state->inputAnonymous)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "500 You'v already input username, please input your password!\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 判断是否是anonymous用户
    if (strcmp(cmd->arg, "anonymous") != 0)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "500 Invalid User.\r\n") < 0)
        {
            return -1;
        };
        return 0;
    }

    // 回应信息
    if (writeCertainSentence(state->connection, buffer,
                             "331 Guest login ok, send your complete e-mail address as password.\r\n") < 0)
    {
        return -1;
    }

    state->inputAnonymous = 1;
    return 0;
}

// 输入密码
int ftpPASS(Command *cmd, State *state, char *buffer)
{
    // 判断是否登陆
    if (state->logged_in)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "500 You'v already logged in.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 判断是否已经输入用户名
    if (!state->inputAnonymous)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "500 Please input your username first!\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 回应信息
    if (writeCertainSentence(state->connection, buffer,
                             "230 Guest login ok, access restrictions apply.\r\n") < 0)
    {
        return -1;
    }

    state->logged_in = 1;
    return 0;
}

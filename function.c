#include "common.h"

// 全局变量
extern char *rootPath;

// 命令列表映射
const char *cmdName[] = {
    "USER", "PASS", "RETR", "STOR", "QUIT", "SYST",
    "TYPE", "PORT", "PASV", "MKD", "CWD", "PWD", "LIST",
    "RMD", "RNFR", "RNTO"};

// 欢迎信息
const char *welcome = "Anonymous FTP server ready.\r\n";

// 日志文件
const char *logFile = "./log";

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
    writeLog(cmdString);
    return sscanf(cmdString, "%s %s", cmd->command, cmd->arg);
}

// 写日至文件
void writeLog(char *cmdString)
{
    // 写入时间
    static int writeTime = 0;

    // 打开文件
    FILE *file = fopen(logFile, "a");
    if (file == NULL)
    {
        perror("Unable to open log file!");
        return;
    }

    if (!writeTime)
    {
        // 写时间
        time_t now;
        struct tm *tm_now;
        time(&now);
        tm_now = localtime(&now);
        fprintf(file, "\n%d-%d-%d %d:%d:%d\n", tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
        writeTime = 1;
    }

    // 写命令记录
    fprintf(file, "%s\n", cmdString);

    // 关闭文件
    fclose(file);
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
    case SYST:
        code = ftpSYST(cmd, state, buffer);
        break;
    case TYPE:
        code = ftpTYPE(cmd, state, buffer);
        break;
    case PORT:
        code = ftpPORT(cmd, state, buffer);
        break;
    case PASV:
        code = ftpPASV(cmd, state, buffer);
        break;
    case RETR:
        code = ftpRETR(cmd, state, buffer);
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

// 显示系统信息
int ftpSYST(Command *cmd, State *state, char *buffer)
{
    // 判断是否登陆
    if (!state->logged_in)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "530 Not logged in.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 回应信息
    if (writeCertainSentence(state->connection, buffer,
                             "215 UNIX Type: L8\r\n") < 0)
    {
        return -1;
    }
    return 0;
}

// 设置TYPE
int ftpTYPE(Command *cmd, State *state, char *buffer)
{
    // 判断是否登陆
    if (!state->logged_in)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "530 Not logged in.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 判断是否是TYPE I
    if (strcmp(cmd->arg, "I") != 0)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "504 Not supported parameter.\r\n") < 0)
        {
            return -1;
        };
        return 0;
    }

    // 回应消息
    if (writeCertainSentence(state->connection, buffer,
                             "200 Type set to I.\r\n") < 0)
    {
        return -1;
    }
    return 0;
}

// 设置主动模式
int ftpPORT(Command *cmd, State *state, char *buffer)
{
    // 判断是否登陆
    if (!state->logged_in)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "530 Not logged in.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    int ip1, ip2, ip3, ip4, p1, p2;
    if (sscanf(cmd->arg, "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &p1, &p2) != 6)
    {
        return -1;
    }
    state->mode = 0;
    sprintf(state->ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    state->port = p1 * 256 + p2;

    // 回应消息
    if (writeCertainSentence(state->connection, buffer,
                             "200 PORT command successful.\r\n") < 0)
    {
        return -1;
    }
    return 0;
}

// 设置被动模式
int ftpPASV(Command *cmd, State *state, char *buffer)
{
    // 判断是否登陆
    if (!state->logged_in)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "530 Not logged in.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    int port1 = 128 + (rand() % 64);
    int port2 = rand() % 256;
    int ip1, ip2, ip3, ip4;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getsockname(state->connection, (struct sockaddr *)&addr, &addr_size);
    sscanf(inet_ntoa(addr.sin_addr), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

    strcpy(buffer, "227 Entering Passive Mode (");
    sprintf(buffer + strlen(buffer), "%d,%d,%d,%d,%d,%d)\r\n", ip1, ip2, ip3, ip4, port1, port2);

    if (writeSentence(state->connection, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }
    if ((state->passive_connection = initializeListenSocket(port1 * 256 + port2)) < 0)
    {
        return -1;
    }
    state->mode = 1;
    return 0;
}

// 下载
int ftpRETR(Command *cmd, State *state, char *buffer)
{
    // 判断是否登陆
    if (!state->logged_in)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "530 Not logged in.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 判断是否确定模式
    if (state->mode == -1)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "500 Not PORT NOR PASS.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    int conn;                // 连接
    struct sockaddr_in addr; // 地址
    char fileName[MAXCMD];   // 文件名
    int file;                // 文件
    int sendBytes;           // 传输的字节
    struct stat stat_buf;    // buf
    off_t off = 0;           // off

    strcpy(fileName, rootPath);
    strcat(fileName, "/");
    strcat(fileName, cmd->arg);

    if ((access(fileName, R_OK) == 0) && (file = open(fileName, O_RDONLY)))
    {
        fstat(file, &stat_buf);
        // 回应150
        if (writeCertainSentence(state->connection, buffer,
                                 "150 Opening BINARY mode data connection.\r\n") < 0)
        {
            return -1;
        }
        /* 主动模式 */
        if (state->mode == 0)
        {
            //创建socket
            if ((conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
            {
                printf("Error socket(): %s(%d)\n", strerror(errno), errno);
                return 1;
            }
            //设置目标主机的ip和port
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(state->port);
            if (inet_pton(AF_INET, state->ip, &addr.sin_addr) <= 0)
            { //转换ip地址:点分十进制-->二进制
                return -1;
            }
            // 连接目标主机
            if (connect(conn, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                return -1;
            }
            if (sendBytes = sendfile(conn, file, &off, stat_buf.st_size))
            {
                if (sendBytes != stat_buf.st_size)
                {
                }
                else
                {
                    if (writeCertainSentence(state->connection, buffer,
                                             "226 Transfer complete.\r\n") < 0)
                    {
                        return -1;
                    }
                }
            }
            else
            {
                if (writeCertainSentence(state->connection, buffer,
                                         "550 File unavailable.\r\n") < 0)
                {
                    return -1;
                }
            }
        }
        close(file);
        close(conn);
    }
    else
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 File unavailable.\r\n") < 0)
        {
            return -1;
        }
    }
}

#include "common.h"

// 全局变量
extern char *rootPath;
extern char *logFile;

// 命令列表映射
const char *cmdName[] = {
    "USER", "PASS", "RETR", "STOR", "QUIT", "SYST",
    "TYPE", "PORT", "PASV", "MKD", "CWD", "PWD", "LIST",
    "RMD", "RNFR", "RNTO", "DELE"};

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
int parseCmd(Command *cmd, char *cmdString, char *log)
{
    memset(cmd->command, 0, 5);
    memset(cmd->arg, 0, MAXCMD);
    strcat(log, cmdString);
    strcat(log, "\n");
    return sscanf(cmdString, "%s %s", cmd->command, cmd->arg);
}

// 写日至文件
void writeLog(char *log)
{
    // 打开文件
    FILE *file = fopen(logFile, "a");
    if (file == NULL)
    {
        perror("Unable to open log file!");
        return;
    }

    // 写时间
    time_t now;
    struct tm *tm_now;
    time(&now);
    tm_now = localtime(&now);
    fprintf(file, "%d-%d-%d %d:%d:%d\n", tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

    // 写命令记录
    fprintf(file, "%s\n", log);

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
    return writeSentence(connfd, buffer, strlen(buffer));
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
    while (buffer[p - 1] == '\r' || buffer[p - 1] == '\n')
    {
        p--;
    }
    buffer[p] = '\0';
    return p;
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
    case STOR:
        code = ftpSTOR(cmd, state, buffer);
        break;
    case QUIT:
        code = ftpQUIT(cmd, state, buffer);
        break;
    case MKD:
        code = ftpMKD(cmd, state, buffer);
        break;
    case CWD:
        code = ftpCWD(cmd, state, buffer);
        break;
    case PWD:
        code = ftpPWD(cmd, state, buffer);
        break;
    case LIST:
        code = ftpLIST(cmd, state, buffer);
        break;
    case RMD:
        code = ftpRMD(cmd, state, buffer);
        break;
    case RNFR:
        code = ftpRNFR(cmd, state, buffer);
        break;
    case RNTO:
        code = ftpRNTO(cmd, state, buffer);
        break;
    case DELE:
        code = ftpDELE(cmd, state, buffer);
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
    // 发送字符串到socket
    if (writeSentence(state->connection, buffer, strlen(buffer)) < 0)
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

    // 关闭连接
    close(state->passive_connection);

    // 解码保存到状态
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

    // 关闭之前的连接
    close(state->passive_connection);

    // 初始化句子信息
    int port1 = 128 + (rand() % 64);
    int port2 = rand() % 256;
    int ip1, ip2, ip3, ip4;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getsockname(state->connection, (struct sockaddr *)&addr, &addr_size);

    sscanf(inet_ntoa(addr.sin_addr), "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
    strcpy(buffer, "227 Entering Passive Mode (");
    sprintf(buffer + strlen(buffer), "%d,%d,%d,%d,%d,%d)\r\n", ip1, ip2, ip3, ip4, port1, port2);

    // 建立连接
    if ((state->passive_connection = initializeListenSocket(port1 * 256 + port2)) < 0)
    {
        return -1;
    }
    state->mode = 1;

    // 回应消息
    if (writeSentence(state->connection, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }

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
                                 "425 Not PORT NOR PASS.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    int conn;                // 连接
    struct sockaddr_in addr; // 地址
    int file;                // 文件
    int sendBytes;           // 传输的字节
    struct stat stat_buf;    // buf

    // 文件路径需要加上文件夹名
    // strcpy(fileName, rootPath);
    // strcat(fileName, "/");
    // strcat(fileName, cmd->arg);

    // 首先判断权限与打开文件流
    if ((access(cmd->arg, R_OK) != -1) && (file = open(cmd->arg, O_RDONLY)) != -1)
    {
        /* 主动模式 */
        if (state->mode == 0)
        {
            //创建socket
            if ((conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
            {
                close(file);
                return -1;
            }
            //设置目标主机的ip和port
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(state->port);
            if (inet_pton(AF_INET, state->ip, &addr.sin_addr) <= 0)
            { //转换ip地址:点分十进制-->二进制
                close(file);
                return -1;
            }
            // 连接目标主机
            if (connect(conn, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                close(file);
                return -1;
            }
        }
        /* 被动模式 */
        else
        {
            conn = accept(state->passive_connection, NULL, NULL);
            close(state->passive_connection);
        }
        // 回应150
        if (writeCertainSentence(state->connection, buffer,
                                 "150 Opening BINARY mode data connection.\r\n") < 0)
        {
            return -1;
        }
        // 传输文件
        fstat(file, &stat_buf);
        if (sendBytes = sendfile(conn, file, NULL, stat_buf.st_size))
        {
            if (sendBytes != stat_buf.st_size)
            {
                if (writeCertainSentence(state->connection, buffer,
                                         "550 File unavailable.\r\n") < 0)
                {
                    close(file);
                    close(conn);
                    return -1;
                }
            }
            else
            {
                if (writeCertainSentence(state->connection, buffer,
                                         "226 Transfer complete.\r\n") < 0)
                {
                    close(file);
                    close(conn);
                    return -1;
                }
            }
        }
        // 传输失败
        else
        {
            if (writeCertainSentence(state->connection, buffer,
                                     "550 File unavailable.\r\n") < 0)
            {
                close(file);
                close(conn);
                return -1;
            }
        }
        close(file);
        close(conn);
    }
    // 文件问题
    else
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 File unavailable.\r\n") < 0)
        {
            return -1;
        }
    }
    state->mode = -1;
    return 0;
}

// 上传
int ftpSTOR(Command *cmd, State *state, char *buffer)
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
                                 "425 Not PORT NOR PASS.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    int conn;                // 连接
    struct sockaddr_in addr; // 地址
    int file;                // 文件
    int pipefd[2];           // 管道

    // 文件路径加上文件夹名
    // strcpy(fileName, rootPath);
    // strcat(fileName, "/");
    // strcat(fileName, cmd->arg);

    // 创建文件
    FILE *fp = fopen(cmd->arg, "w");
    fclose(fp);

    // 先判断文件权限
    if ((access(cmd->arg, W_OK) != -1) && (file = open(cmd->arg, O_WRONLY)) != -1)
    {
        /* 主动模式 */
        if (state->mode == 0)
        {
            //创建socket
            if ((conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
            {
                close(file);
                return -1;
            }
            //设置目标主机的ip和port
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(state->port);
            if (inet_pton(AF_INET, state->ip, &addr.sin_addr) <= 0)
            { //转换ip地址:点分十进制-->二进制
                close(file);
                return -1;
            }
            // 连接目标主机
            if (connect(conn, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                close(file);
                return -1;
            }
        }
        /* 被动模式 */
        else
        {
            conn = accept(state->passive_connection, NULL, NULL);
            close(state->passive_connection);
        }
        // 回应125
        if (writeCertainSentence(state->connection, buffer,
                                 "125 Data connection already open; transfer starting.\r\n") < 0)
        {
            close(file);
            return -1;
        }
        // 传输文件
        if (pipe(pipefd) == -1)
        {
            if (writeCertainSentence(state->connection, buffer,
                                     "550 Pipe error.\r\n") < 0)
            {
                close(file);
                close(conn);
                return -1;
            }
        }
        else
        {
            int res = 1;
            while (res = splice(conn, 0, pipefd[1], NULL, 8192, SPLICE_F_MORE | SPLICE_F_MOVE) > 0)
            {
                splice(pipefd[0], NULL, file, 0, 8192, SPLICE_F_MORE | SPLICE_F_MOVE);
            }
            if (res == -1)
            {
                close(file);
                close(conn);
                return -1;
            }
            else
            {
                if (writeCertainSentence(state->connection, buffer,
                                         "226 Transfer complete.\r\n") < 0)
                {
                    close(file);
                    close(conn);
                    return -1;
                }
            }
        }
        close(file);
        close(conn);
    }
    // 文件错误
    else
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 File unavailable.\r\n") < 0)
        {
            return -1;
        }
    }
    state->mode = -1;
    return 0;
}

// 退出
int ftpQUIT(Command *cmd, State *state, char *buffer)
{
    // 回应消息
    if (writeCertainSentence(state->connection, buffer,
                             "221 GoodBye.\r\n") < 0)
    {
        return -1;
    }
    return 1;
}

// 创建文件夹
int ftpMKD(Command *cmd, State *state, char *buffer)
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

    // 禁止绝对路径
    if (cmd->arg[0] == '/')
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to create directory. Check path or permissions.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 创建文件夹
    // 获取当前路径
    char cwd[MAXCMD];
    if (getcwd(cwd, MAXCMD) == NULL)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to create directory. Check path or permissions.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    };
    if (mkdir(cmd->arg, S_IRWXU) == 0)
    {
        sprintf(buffer, "257 \"%s/%s\" directory created.\r\n", cwd, cmd->arg);
        if (writeSentence(state->connection, buffer, strlen(buffer)) < 0)
        {
            return -1;
        }
    }
    else
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to create directory. Check path or permissions.\r\n") < 0)
        {
            return -1;
        }
    }

    return 0;
}

// 进入文件夹
int ftpCWD(Command *cmd, State *state, char *buffer)
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

    // 禁止绝对路径
    if (cmd->arg[0] == '/')
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to change directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    char cwd[MAXCMD];
    if (getcwd(cwd, MAXCMD) == NULL)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to change directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    };

    // 判断..
    if (strcmp(cmd->arg, "..") == 0)
    {
        // 如果已经是在根目录，禁止 cd ..
        if (strcmp(cwd, rootPath) == 0)
        {
            if (writeCertainSentence(state->connection, buffer,
                                     "550 Failed to change directory.\r\n") < 0)
            {
                return -1;
            }
            return 0;
        }
    }

    // 进入文件夹
    if (chdir(cmd->arg) == -1)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to change directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 获取当前路径
    if (getcwd(cwd, MAXCMD) == NULL)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to change directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    };

    // 回应消息
    sprintf(buffer, "250 directory changed to %s\r\n", cwd);
    if (writeSentence(state->connection, buffer, strlen(buffer)) < 0)
    {
        return -1;
    }
    return 0;
}

// 显示当前目录
int ftpPWD(Command *cmd, State *state, char *buffer)
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

    // 获取当前目录
    char cwd[MAXCMD];
    if (getcwd(cwd, MAXCMD) == NULL)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to print directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    };

    // 回应消息
    sprintf(buffer, "257 \"%s\"\r\n", cwd);
    if (writeSentence(state->connection, buffer, strlen(buffer)))
    {
        return -1;
    }
    return 0;
}

// 显示当前目录下文件
int ftpLIST(Command *cmd, State *state, char *buffer)
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

    // 静止根目录访问
    if (strlen(cmd->arg) > 0 && cmd->arg[0] == '/')
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Permission denied.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 原始目录
    char origin_cwd[MAXCMD], cwd[MAXCMD];
    if (getcwd(origin_cwd, MAXCMD) == NULL)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to list directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    };

    // 切换目录
    if (strlen(cmd->arg) > 0)
    {
        if (chdir(cmd->arg) == -1)
        {
            if (writeCertainSentence(state->connection, buffer,
                                     "550 Failed to list directory.\r\n") < 0)
            {
                return -1;
            }
            return 0;
        }
    }

    // 获取切换后的路径
    if (getcwd(cwd, MAXCMD) == NULL)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Failed to list directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    };

    struct sockaddr_in addr; // 地址
    struct dirent *entry;    // 文件夹入口
    struct stat statbuf;
    char per[10];
    int conn;

    // 打开文件夹
    DIR *dir = opendir(cwd);

    // 打开失败
    if (!dir)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 4 Failed to list directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 打开成功

    /* 主动模式 */
    if (state->mode == 0)
    {
        //创建socket
        if ((conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            closedir(dir);
            return -1;
        }
        //设置目标主机的ip和port
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(state->port);
        if (inet_pton(AF_INET, state->ip, &addr.sin_addr) <= 0)
        { //转换ip地址:点分十进制-->二进制
            closedir(dir);
            return -1;
        }
        // 连接目标主机
        if (connect(conn, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            closedir(dir);
            return -1;
        }
    }
    /* 被动模式 */
    else
    {
        conn = accept(state->passive_connection, NULL, NULL);
        close(state->passive_connection);
    }

    // 回应150
    if (writeCertainSentence(state->connection, buffer,
                             "150 Here comes the directory listing.\r\n") < 0)
    {
        closedir(dir);
        return -1;
    }

    buffer[0] = '\0';

    while (entry = readdir(dir))
    {
        if (stat(entry->d_name, &statbuf) == -1)
        {
            continue;
        }
        else
        {
            // 参考博客：https://blog.csdn.net/qq_43648751/article/details/104222145
            // 转换文件权限
            memset(per, 0, 10);
            char str[4];
            int read, write, exec;
            int curper;
            for (int i = 6; i >= 0; i -= 3)
            {
                curper = ((statbuf.st_mode & ALLPERMS) >> i) & 7;
                memset(str, 0, 4);
                read = (curper >> 2) & 1;
                write = (curper >> 1) & 1;
                exec = (curper >> 0) & 1;
                sprintf(str, "%c%c%c", read ? 'r' : '-', write ? 'w' : '-', exec ? 'x' : '-');
                strcat(per, str);
            }

            // 转换文件修改时间
            struct tm *t;
            char time[80];
            time[0] = '\0';
            t = localtime(&statbuf.st_mtime);
            strftime(time, 80, "%b %d %H:%M", t);

            // 输出
            sprintf(buffer + strlen(buffer),
                    "%c%s %5lu %4d %4d %8ld %s %s\r\n",
                    (entry->d_type == DT_DIR) ? 'd' : '-',
                    per, statbuf.st_nlink,
                    statbuf.st_uid,
                    statbuf.st_gid,
                    statbuf.st_size,
                    time,
                    entry->d_name);
        }
    }

    // 返回消息
    if (writeSentence(conn, buffer, strlen(buffer)) < 0)
    {
        close(conn);
        closedir(dir);
        return -1;
    }

    close(conn);
    closedir(dir);
    state->mode = -1;

    // 回应消息
    if (writeCertainSentence(state->connection, buffer,
                             "226 Directory list OK.\r\n") < 0)
    {
        return -1;
    }

    // 返回原来的文件夹
    if (chdir(cmd->arg) == -1)
    {
    };
    return 0;
}

// 删除文件夹
int ftpRMD(Command *cmd, State *state, char *buffer)
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

    // 删除文件夹
    if (rmdir(cmd->arg) == -1)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Cannot delete directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 回应消息
    if (writeCertainSentence(state->connection, buffer,
                             "250 Successfully remove directory.\r\n") < 0)
    {
        return -1;
    }
    return 0;
}

// 重命名FROM
int ftpRNFR(Command *cmd, State *state, char *buffer)
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

    strcpy(state->filename, cmd->arg);
    return 0;
}

// 重命名TO
int ftpRNTO(Command *cmd, State *state, char *buffer)
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

    if (rename(state->filename, cmd->arg) == -1)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Cannot rename file.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    if (writeCertainSentence(state->connection, buffer,
                             "250 Successfully rename file.\r\n") < 0)
    {
        return -1;
    }
    return 0;
}

// 删除文件
int ftpDELE(Command *cmd, State *state, char *buffer)
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

    // 删除文件
    if (unlink(cmd->arg) == -1)
    {
        if (writeCertainSentence(state->connection, buffer,
                                 "550 Cannot delete directory.\r\n") < 0)
        {
            return -1;
        }
        return 0;
    }

    // 回应消息
    if (writeCertainSentence(state->connection, buffer,
                             "250 Successfully delete file.\r\n") < 0)
    {
        return -1;
    }
    return 0;
}
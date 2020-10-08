#ifndef __CUSTOM_H__
#define __CUSTOM_H__

#define _GNU_SOURCE

#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define MAXBUF 1024 * 1024
#define MAXCMD 1024 * 1

// 连接状态
typedef struct
{
    int inputAnonymous;     // 是否输入Anonymous
    int logged_in;          // 1 if logged in
    int mode;               // 0 for port & 1 for pasv
    char ip[17];            // ip地址
    int port;               // portNumber client gives
    int connection;         // connfd
    int passive_connection; // passive_connection
} State;

// 命令
typedef struct
{
    char command[5];
    char arg[MAXCMD];
} Command;

// 命令列表枚举
typedef enum
{
    USER = 0,
    PASS = 1,
    RETR,
    STOR,
    QUIT,
    SYST,
    TYPE,
    PORT,
    PASV,
    MKD,
    CWD,
    PWD,
    LIST,
    RMD,
    RNFR,
    RNTO,
    CMDLISTSIZE,
} CmdList;

// server.c中定义的函数
void argError(int argc, char **argv);                           // 参数错误处理
int dealCmd(int argc, char **argv, int *port, char **rootPath); // 处理输入参数
int initializeListenSocket(int port);                           // 初始化监听Socket
void *createFTP(void *arg);                                     // 创建FTP会话

// function.c中定义的函数
int getIndexInCmdList(char *cmd);                              // 获取命令对应的序号
int parseCmd(Command *cmd, char *cmdString, char *log);        // 解析命令
void writeLog(char *log);                                      // 写日志文件
int writeSentence(int connfd, char *buffer, int len);          // 发送字符串到socket
int writeCertainSentence(int connfd, char *buffer, char *str); // 发送特定字符串
int readSentence(int connfd, char *buffer);                    // 从socket接收字符串
int response(Command *cmd, State *state, char *buffer);        // 返回相应操作

int ftpWelcome(State *state, char *buffer);            // 发送欢迎信息
int ftpUSER(Command *cmd, State *state, char *buffer); // 登陆
int ftpPASS(Command *cmd, State *state, char *buffer); // 输入密码
int ftpSYST(Command *cmd, State *state, char *buffer); // 显示系统信息
int ftpTYPE(Command *cmd, State *state, char *buffer); // 设置TYPE
int ftpPORT(Command *cmd, State *state, char *buffer); // 设置主动模式
int ftpPASV(Command *cmd, State *state, char *buffer); // 设置被动模式
int ftpRETR(Command *cmd, State *state, char *buffer); // 下载
int ftpSTOR(Command *cmd, State *state, char *buffer); // 上传
int ftpQUIT(Command *cmd, State *state, char *buffer); // 退出

#endif
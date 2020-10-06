#include "common.h" // include库文件与函数声明

int port = 21;			 // 默认端口号
char *rootPath = "/tmp"; // 默认根目录文件夹

int main(int argc, char **argv)
{
	if (!dealCmd(argc, argv, &port, &rootPath))
	{
		return 1;
	}; // 处理输入参数

	int listenfd; // 监听ftp请求的socket
	if ((listenfd = initializeListenSocket(port)) < 0)
	{
		return 1;
	}

	// 持续监听FTP连接请求
	while (1)
	{
		// 等待client的连接 -- 阻塞函数
		int connfd = accept(listenfd, NULL, NULL);
		if (connfd == -1)
		{
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}

		pthread_t th;
		if (pthread_create(&th, NULL, createFTP, &connfd) != 0)
		{
			printf("Create thread error!\n");
			return -1;
		}
	}

	close(listenfd);

	printf("port: %d\n", port);
	printf("root: %s\n", rootPath);
	return 0;
}

// 参数错误处理
void argError(int argc, char **argv)
{
	printf("Wrong commands: ");
	for (int i = 0; i < argc; i++)
	{
		printf("%s ", argv[i]);
	}
	printf("\n");
	printf("Usage: server [-port portNum] [-root rootPath]\n");
}

// 处理输入参数
int dealCmd(int argc, char **argv, int *port, char **rootPath)
{

	if (argc > 5)
	{
		argError(argc, argv);
		return 0;
	}
	for (int i = 1; i < argc; i++)
	{
		if (i & 1 != 0)
		{
			if (strcmp(argv[i], "-port") == 0)
			{
				if (i + 1 >= argc)
				{
					argError(argc, argv);
					return 0;
				}
				else
				{
					*port = atoi(argv[i + 1]);
					if (*port <= 0 || *port >= 65536)
					{
						argError(argc, argv);
						return 0;
					}
				}
			}
			else if (strcmp(argv[i], "-root") == 0)
			{
				if (i + 1 >= argc)
				{
					argError(argc, argv);
					return 0;
				}
				else
				{
					*rootPath = argv[i + 1];
				}
			}
			else
			{
				argError(argc, argv);
				return 0;
			}
		}
	}
	return 1;
}

// 初始化监听Socket
int initializeListenSocket(int port)
{
	int listenfd;
	struct sockaddr_in addr; // server地址

	// 创建socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
	{
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	// 设置本机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);			  // 转换为正确的端口号
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听“0.0.0.0”即所有地址

	// 设置
	int reuse = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
	{
		printf("Error setsockopt(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	// 将本机的ip和port与socket绑定
	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	// 开始监听socket
	if (listen(listenfd, 10) == -1)
	{
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return -1;
	}

	return listenfd;
}

// 创建FTP会话
void *createFTP(void *arg)
{
	int connfd = *(int *)arg; // 连接
	char buffer[MAXBUF];	  // 缓存
	State state;			  // 服务状态
	Command cmd;			  // 命令

	memset(buffer, 0, MAXBUF);
	memset(&state, 0, sizeof(State));
	memset(&cmd, 0, sizeof(Command));

	state.mode = -1;
	state.connection = connfd;

	if (ftpWelcome(&state, buffer) < 0)
	{
		close(connfd);
		return NULL;
	}

	while (readSentence(connfd, buffer) >= 0)
	{
		if (parseCmd(&cmd, buffer) == 0)
		{
			continue;
		};
		if (response(&cmd, &state, buffer) < 0)
		{
			close(connfd);
			return NULL;
		}
	}

	close(connfd);
	return NULL;
}
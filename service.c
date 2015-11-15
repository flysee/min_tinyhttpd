#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SERVER_STRING "Server: xx\r\n"

void *accept_request();
void serve_file();
void headers();
void cat();
void execute_cgi();

void *accept_request(int client)
{
	int recvbytes;
	char buf[1024];
	char path[512];
	if((recvbytes=recv(client,buf,1024,0))==-1)  
	{  
		perror("recv error");exit(1);  
	}
	buf[recvbytes]='\0';
	printf("%s\n", buf);
	/*
	这里输出，不对请求做出处理，直接测试打开静态和cgi
	*/
	// strcat(path, "index.html");
	// serve_file(client, path);
	strcat(path, "cgi.py");
	execute_cgi(client,path);
	close(client);
}

void serve_file(int client,const char *filename)
{
	FILE *resource = NULL;
	char buf[1024];
	resource=fopen(filename,"r");
	if (resource == NULL)
	{
		perror("not found");exit(1);
	}
	else
	{
		headers(client);
		cat(client,resource);
	}
	fclose(resource);
}

void execute_cgi(int client, const char *path)
{
	// char buf[1024];
    int cgi_output[2];
    // int cgi_input[2];
    int status;
    char c;
    pid_t pid;
   	// headers(client);
   	/* 建立管道*/
    if (pipe(cgi_output)<0)
    {
    	perror("pipe error");
    	exit(0);
    }

    if ((pid=fork())<0)
    {
    	perror("fork error");
    	exit(0);
    }
    if (pid==0)
    {
    	char meth_env[255];
      char query_env[255];
     /* 把 STDOUT 重定向到 cgi_output 的写入端 */
    	dup2(cgi_output[1],1);
    	close(cgi_output[0]);

		/*设置 request_method 的环境变量*/
        sprintf(meth_env, "REQUEST_METHOD=%s","method");
        putenv(meth_env);
        sprintf(query_env, "QUERY_STRING=%s","query_string");
        putenv(query_env);
/*执行cgi脚本*/
    	execl(path,NULL);
    	printf("path=%s",path);
    
    	exit(0);
    }
    else
    {
    	close(cgi_output[1]);
    	while(read(cgi_output[0],&c,1)>0)
    		send(client,&c,1,0);
    	// printf("success\n");
    	close(cgi_output[0]);
    	waitpid(pid,&status,0);
    }
}

void headers(int client)
{
	char buf[1024];
	strcpy(buf,"HTTP/1.0 200 OK\r\n");
	send(client,buf,strlen(buf),0);

	strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

void cat(int client,FILE *resource)
{
	char buf[1024];

	fgets(buf,sizeof(buf),resource);
	while(!feof(resource))
	{
		send(client,buf,strlen(buf),0);
		fgets(buf,sizeof(buf),resource);
	}
}

int main(int argc, char const *argv[])
{
	int sockfd,client_fd;
	u_short port=8888;
	struct sockaddr_in my_addr,it_addr;
	if ((sockfd=socket(PF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket error");exit(1);
	}
	memset(&my_addr,0,sizeof(my_addr));
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(port);
	my_addr.sin_addr.s_addr=htonl(INADDR_ANY);

	if (bind(sockfd,(struct sockaddr *)&my_addr, sizeof(my_addr))==-1)
	{
		perror("bind error");exit(1);
	}

	if (listen(sockfd,5)==-1)  
	{  
		perror("listen error");exit(1);  
	}
	int sin_size= sizeof (my_addr);  
	pthread_t newthread;
	while(1)
	{
		if ((client_fd=accept(sockfd,(struct sockaddr *)&it_addr,&sin_size))==-1)  
		{  
			perror("accept error");exit(1);
		}
		/*创建线程*/
		if (pthread_create(&newthread , NULL, (void *)accept_request, (int *)client_fd) != 0)
			perror("pthread_create");
	}
	close(sockfd);
	return 0;
}

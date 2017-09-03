#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


typedef struct LogHead LogHead;//头部	24字节
struct LogHead{	
	int size;		//日志长度	
	char type[20];	//类型	
	char log[1024];	//日志
	};


int main(int argc, char *argv[])
{

	LogHead logd;
	logd.size = 102;
	memcpy(logd.type,"type_pages",sizeof("type_pages"));
	memcpy(logd.log,"whole{povoe,jack,moere:ds,mawe:sdad}iaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiimmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",sizeof("whole{povoe,jack,moere:ds,mawe:sdad}iaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiimmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"));
	
	char snd_buff[1024];
	memcpy(snd_buff,&logd,sizeof(logd));
	

	daemon(1,1);
	int sock;
	//sendto中使用的对方地址
	struct sockaddr_in toAddr;
	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock < 0)
	{
 		printf("创建套接字失败了.\r\n");
 		exit(1);
	}
	memset(&toAddr,0,sizeof(toAddr));
	toAddr.sin_family=AF_INET;
	toAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	toAddr.sin_port = htons(51999);
	
	//int num = 180000;
	int num =80;
	while(num--)
	{
	
		if(sendto(sock,snd_buff,sizeof(snd_buff),0,(struct sockaddr*)&toAddr,sizeof(toAddr)) < 0)
		{
 			printf("sendto() 函数使用失败了.\r\n");
 			close(sock);
 			exit(1);
		}
		
		int mi = 0;
		int i = 0;
		int j = 0;
		for(i ; i < 10000;i++)
		{
			for(j ;j < 10000;j++)
			{
				mi = i * j;
			}
		}
		if(num%100 == 0)
		{
			sleep(1);
		}
	}
	close(sock);
	printf("done!!!\n");
}

#include<stdlib.h>
#include<stdio.h>
#include<proto.h>
#include "client.h"
#include<site_type.h>
#include<errno.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<getopt.h>


struct client_conf_structure client_conf={.rcvport = DEFAULT_RCVPORT,\
							.mgroup = DEFAULT_MGROUP,\
							.player_cmd = DEFAULT_PLAYERCMD};//用户的默认信息，可以通过终端命令行输入命令修改，通过getopt_long函数实现

static void printhelp(void){//这个函数用来打印帮助信息，以免新用户不知道如何操作
	printf("-P --port    select receiver port");
	printf("-M --mgroup  select multigroup");
	printf("-p --player  select player commandline");
	printf("-H --help    display help information");
}

static ssize_t writen(int fd,const void* vptr,size_t n){//这个函数是确保一定能够读够n个内容
	size_t nleft;
	ssize_t nwritten;//已经写了的数量
	const char* ptr;
	ptr = vptr;
	nleft = n;
	while(nleft > 0){
		if((nwritten = write(fd, ptr, nleft)) <= 0){
			if(nwritten < 0 && errno == EINTR)//被信号打断需要重新尝试
				nwritten = 0;
			else
				return (-1);		
		}
		nleft -= nwritten;
		ptr += nwritten; 	
	}
	return (n);
}

int main(int argc,char ** argv){
	int index = 0;
	int c;
	int sd;
	int val,chosenid;
	int pd[2];
	int len;
	pid_t pid;
	struct sockaddr_in localaddr,serveraddr,raddr;
	socklen_t serveraddr_len,raddr_len;
	struct ip_mreqn mreq;	
	struct option argarr[] = {{"port",1,NULL,'P'},\
					{"mgroup",1,NULL,'M'},\
					{"player",1,NULL,'p'},\
					{"help",0,NULL,'H'}};//同时实现了短命令（-）和长命令（--）
	while(1){	
		c = getopt_long(argc,argv,"P:M:p:H",argarr,&index);//这个部分使用户在命令行输入自己想加入的多播组和使用的解码器
		if(c < 0)
			break;
		switch(c){
			case 'P':
				client_conf.rcvport = optarg;	
				break;
			case 'M':
				client_conf.mgroup = optarg;	
				break;
			case 'p':
				client_conf.player_cmd = optarg;	
				break;
			case 'H':
				printhelp();
				exit(0);	
				break;
			default:
				abort();
				break;	
		}
	}
	sd = socket(AF_INET,SOCK_DGRAM,0);//多播组是一对多，需要使用UDP传输
	if(sd < 0){
		perror("socket()");
		exit(1);		
	}
		
	inet_pton(AF_INET,client_conf.mgroup,&mreq.imr_multiaddr);
	inet_pton(AF_INET,"0.0.0.0",&mreq.imr_address);
	mreq.imr_ifindex = if_nametoindex("eth0");//设置多播组信息，，设置socket选项，，加入多播组		
		
	if(setsockopt(sd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0){
		perror("setsockopt()");
		exit(1);
	}

	val = 1;
	if(setsockopt(sd,IPPROTO_IP,IP_MULTICAST_LOOP,&val,sizeof(val)) < 0){
		perror("setsockopt()");
		exit(1);
	}

	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(atoi(client_conf.rcvport));
	inet_pton(AF_INET,"0.0.0.0",&localaddr.sin_addr);

	if(bind(sd,(void*)&localaddr,sizeof(localaddr)) < 0){
		perror("bind()");
		exit(1);		
	}//客户端可以不BIND


	if(pipe(pd) < 0){
		perror("pipe()");
		exit(1);
	}//我们这里用管道进行进程间的通信，如果在父进程使用exec函数，那么之前的socket连接工作就白做了，所以这个要在子进程进行

	pid = fork();
	if(pid < 0){
		perror("fork()");
		exit(1);		
	} 
	if(pid == 0){//这块是子进程
		close(sd);
		close(pd[1]);//子进程关闭写端，只需要读
		dup2(pd[0],0);//dup2 函数用于复制文件描述符，将 pd[0] 复制到文件描述符 0（标准输入）。这样做的效果是，子进程的标准输入将从管道 pd 的读取端获取数据。
                 //pd[0] 是管道的读取端，它接收父进程通过管道发送的数据，而标准输入通常用于从键盘读取数据，但在这里被重定向到管道，以便子进程可以读取父进程发送的数据。
		if(pd[0] > 0)
			close(pd[0]);
		execl("/bin/sh","sh","-c",client_conf.player_cmd,NULL);	
		perror("execl()");
		exit(1);			
	}

	struct msg_list_struct* list_rcv;
	list_rcv = malloc(MSG_LIST_MAX);//这里为接受到的节目单列表信息包准备空间接受
	if(list_rcv == NULL){
		perror("malloc()");
		exit(1);		
	}
	while(1){
		len = recvfrom(sd,list_rcv,MSG_LIST_MAX,0,(void*)&serveraddr,&serveraddr_len);
		if(len < sizeof(struct msg_list_struct)){
			fprintf(stderr,"msg message is too small.\n");
			continue;		
		} 		
		if(list_rcv->chanid != LISTENCHANID){//刚开始一定要客户收到到节目单，告诉客户哪个频道是什么节目
			continue;		
		}
		break;
	}

	struct msg_listitem_struct* pos;
	for(pos = list_rcv->category;pos < ((char*)list_rcv)+len;pos = (void*)(((char*)pos)+ntohs(pos->len))){
		printf("channel %d : %s\n",pos->chanid,pos->description);		
	}//根据我们定义的包的信息打印节目单和内容介绍，方便客户作出选择
	int ret;
	free(list_rcv);
	while(1){
		ret = scanf("%d",&chosenid);//用户输入自己的选择
		if(ret != 1)
			exit(1);
		break;				
	}
	
	struct msg_chan_struct *channel_rcv;//为即将接受到所选择的频道的内容建立缓冲区
	channel_rcv = malloc(MSG_CHAN_MAX);
	if(channel_rcv == NULL){
		perror("malloc()");
		exit(1);	
	}

	while(1){//只用频道号是自己选择的包才播放，否则忽略
		len = recvfrom(sd,channel_rcv,MSG_CHAN_MAX,0,(void*)&raddr,&raddr_len);
		if(raddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr || raddr.sin_port != serveraddr.sin_port){
			fprintf(stderr,"Ignore:address not match.\n");//可以注释掉，这里写这句话只是为了调试
			continue;		
		}
		if(len < sizeof(struct msg_chan_struct)){
			fprintf(stderr,"Ignore:message not match.\n");//同理
			continue;		
		}
		if(channel_rcv->chanid == chosenid){
			fprintf(stdout,"accepted message: %d received.\n",channel_rcv->chanid);
			writen(pd[1],channel_rcv->data,len - sizeof(chanid_t));		
		}		
	}

	free(channel_rcv);
	close(sd);

	exit(0);
	
}

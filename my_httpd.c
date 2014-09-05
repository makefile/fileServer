#include"my_httpd.h"
char ip[128],port[8];//全是字符串
char back[8],home_dir[128];//listen队列大小，浏览主目录
char upload_root[128];
int main(int argc,char **argv){
	struct sockaddr_in addr;
	int sock_fd;
	unsigned int addrlen;
//	init_daemon(argv[0],LOG_INFO);//运行守护进程
	if(get_arg("home_dir")==0)//从配置文件读取参数
		sprintf(home_dir,"%s","/tmp");
	if(get_arg("upload_dir")==0) 
		sprintf(upload_root,"%s","/var/www");
	if(get_arg("ip")==0) get_addr("eth0");//本机ip
	if(get_arg("port")==0) sprintf(port,"%s","80");//默认80
	if(get_arg("back")==0) sprintf(back,"%s","5");
	if((sock_fd=socket(PF_INET,SOCK_STREAM,0))<0){
		info("socket()");//对syslog(LOG_INFO,"%s",msg)的包装
		exit(-1);
	}
	addrlen=1;//any integer,value-result argument
	setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&addrlen,sizeof(addrlen));//容许重用本地地址和端口
	addr.sin_family=AF_INET;
	addr.sin_port=htons(atoi(port));
	addr.sin_addr.s_addr=inet_addr(ip);
	addrlen=sizeof(struct sockaddr_in);
	if(bind(sock_fd,(struct sockaddr*)&addr,addrlen)<0){
		info("bind");exit(-1);
	}
	if(listen(sock_fd,atoi(back))<0){
		info("listen");exit(-1);
	}
	while(1){
		int len,new_fd;
		//addrlen=sizeof(struct sockaddr_in);
		new_fd=accept(sock_fd,(struct sockaddr*)&addr,&addrlen);
		if(new_fd<0){
			info("accept");
			exit(-1);
		}
		bzero(buffer,MAXBUF+1);//似乎buffer,MAXBUF都在标准库,对超出的内存区域初始化会段错误，不知+1后会不会超出
		//in strings.h,but memset in string.h
		sprintf(buffer,"connect come from: %s:%d\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		info(buffer);//log
		pid_t pid;
		int contLen=0;
		if((pid=fork())==-1){
			info("fork");
			exit(-1);
		}
		if(pid==0){//child
			close(sock_fd);
			bzero(buffer,MAXBUF+1);
			if((len=recv(new_fd,buffer,MAXBUF,0))>0){
				FILE *clientFP=fdopen(new_fd,"w");//care!
				//获取文件描述符，能够使用fprintf,但不使用fread,因为难以把控
				if(clientFP==NULL){
					info("fdopen");
					exit(-1);
				}else{
					char Req[MAXPATH+1]="";
					if(strstr(buffer,"GET ")!=NULL){
						sscanf(buffer,"GET %s HTTP",Req);
						bzero(buffer,MAXBUF+1);
						sprintf(buffer,"Request get the file: \"%s\"\n",Req);
						info(buffer);
						GiveResponse(clientFP,Req);//专门响应连接
					}else if(strncmp(buffer,"POST",4)==0){
					//	info(buffer);
						sscanf(buffer,"POST %s HTTP",Req);
						sscanf(strstr(buffer,"Content-Length")
								,"Content-Length: %d",&contLen);
						//notice there is a space after ':',sscanf须从第一个参数的地址开始处匹配，不能智能地搜寻自己想要的片段
						bzero(buffer,MAXBUF+1);
						sprintf(buffer,"Request POST: \"%s\",len:%d\n",Req,contLen);
						info(buffer);
						//recv(new_fd,buffer,MAXBUF,0);
						//上传文件时浏览器向连接发送了三次数据，第一次是请求头信息，第二次是文件名等相关信息，第三次才是文件内容。需要三次读。
						sscanf(strstr(buffer,"Boundary=")
								,"Boundary= %s",buffer);
						info(buffer);
						uploadFile(new_fd,Req,contLen);
					}
					fclose(clientFP);
				}
			}
			exit(0);
		}
		else{
			close(new_fd);
			continue;
		}
	}
	close(sock_fd);
	return 0;
}




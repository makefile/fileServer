#include "my_httpd.h"
char *formatText(char *text);
/*
   以html响应客户端的请求，若请求是目录，则列出目录信息，若是文件，则将文件内容传送给客户端
   */
int hexstr2int(char x,char y){
	int ret=16776960,i,tmp=0;//0xffff00
	char a[]={x,y};
	for(i=0;i<2;i++){
		if(a[i]>='0'&&a[i]<='9') tmp=tmp*16+a[i]-'0';
		else tmp=tmp*16+a[i]-55;//-65+10 ascii:A65,a97
		//Path中的中文编码是大写的，如'传':%E4%BC%A0,实际为16进制0xFFFFE4...
	}
	ret=ret+tmp;
	return ret;
}
/*返回文件的后缀名，此处仅简单判断最后的.后的字符，因此不适用于.tar.gz等以及后缀名与属性不符的情形，后续改进 */
char *postfix(char *file){
	int i=0,lastIndex=0;
	while(file[i]!='\0'){
		if(file[i]=='.') lastIndex=i;
		i++;
	}
	return &file[lastIndex+1];
}
void GiveResponse(FILE *client_sock,char *path){
	struct dirent *dirent;
	struct stat fileinfo;
	char Filename[MAXPATH];
	DIR *dir;
	int fd,len,ret,i=0;
	//FILE *fp;
	char *p,*realpath,*realFilename,*nport;
	struct passwd *p_passwd;
	struct group *p_group;
	char chinese[128];
	extern char home_dir[],ip[],port[];
	//将中文名还原成主机能够识别的编码
	for(ret=0,len=strlen(path);ret<len;ret++){
		if(path[ret]=='%') {
			path[ret+2]=hexstr2int(path[ret+1],
					path[ret+2]);
			ret+=2;
		}
	}
	for(ret=0;ret<len;ret++){
		if(path[ret]=='%'){
			chinese[i++]=path[ret+2];
			ret+=2;
		}else
			chinese[i++]=path[ret];
	}
	chinese[i]='\0';
	if(i>0) strcpy(path,chinese);
	//因请求的文件是以服务器主目录为根目录如/var/www/,因此需要加该根目录才是绝对路径
	len=strlen(home_dir)+strlen(path)+1;
	realpath=malloc(len+1);
	bzero(realpath,len+1);
	sprintf(realpath,"%s/%s",home_dir,path);
	//get port
	len=strlen(port)+1;
	nport=malloc(len+1);
	bzero(nport,len+1);
	sprintf(nport,":%s",port);
	//file state
	if(stat(realpath,&fileinfo)){//fail
		fprintf(client_sock,"HTTP/1.1 200 OK\r\nServer:Test http server\r\nConnection:close\r\n\r\n"//两个\r\n,头部结束，HTTP1.0默认close,1.1默认keep-alive
			"<html lang=\"zh-cn\"><html><head><meta charset=\"utf-8\"><title>%d - %s</title></head>"
			"<body><font size=+4>Linux HTTP server</font><br><hr width=\"100%%\"><br><center><table border cols=3 width=\"100%%\"></table><font color=\"CC0000\" size=+2> connect to administrator,error path is: \n%s %s</font></body></html>",errno,strerror(errno),path,strerror(errno));
		goto out;
	}
	if(S_ISREG(fileinfo.st_mode)){//send file content
		fd=open(realpath,O_RDONLY);
		len=lseek(fd,0,SEEK_END);
		p=(char *)malloc(len+2048);//plus 2048 is due to i when display the text,need extra space:'<' changes to &#60;
		bzero(p,len+2048);
		lseek(fd,0,SEEK_SET);
		ret=read(fd,p,len);//一次性读取文件，对于大文件会出错，有时间再改，分批读取和传送文件
		char logmsg[30];
		if(ret<0) info("read fail");
		else {
			sprintf(logmsg,"read len=%d,the real len is %d\n",ret,len);
			info(logmsg);
		}
		close(fd);
		if(strcmp("c",postfix(path))==0
				||strcmp("txt",postfix(path))==0){
		//<的转义序列为&lt; or &#60;,> &gt; &#62;,&的转义为&amp; or &#38; 不转义的话<stdio.h>被当作标签，但不显示
			fprintf(client_sock,"HTTP/1.1 200 OK\r\nServer:Test http server\r\nConnection:close\r\n\r\n<html lang=\"zh-cn\"><html><head><meta charset=\"utf-8\"><title>content of %s</title></head><body><font size=+4>康哥's file</font><br><hr width=\"100%%\"><br>%s</body></html>"//<center>
					,path,formatText(p));
		}else{
			fprintf(client_sock,"HTTP/1.1 200 OK\r\nServer:Test http server\r\nConnection:keep-alive\r\nContent-type:application/*\r\nContent-Length:%d\r\n\r\n",len);
			fwrite(p,len,1,client_sock);//send file content
		}
		free(p);
	}else if(S_ISDIR(fileinfo.st_mode)){
		dir=opendir(realpath);
		fprintf(client_sock,"HTTP/1.1 200 OK\r\nServer:Test http server\r\nConnection:close\r\n\r\n<html lang=\"zh-cn\"><html><head><meta charset=\"utf-8\"><title>%s</title></head><body><font size=+4>康哥's file</font><br><hr width=\"100%%\"><br><center>"
			"<table border cols=3 width=\"100%%\">",path);
		fprintf(client_sock,"<caption><font size=+3> Directory %s</font></caption>\n",path);//表格头信息，便于显示
		fprintf(client_sock,"<tr><td>name</td><td>type</td><td>owner</td><td>group</td><td>size</td><td>modify time</td></tr>\n");
		if(dir==NULL){//打开目录失败
			fprintf(client_sock,"</table><font color=\"CC0000\" size=+2>%s</font></body></html>",strerror(errno));
			return ;
		}
		fprintf(client_sock,"<td><a href=\"http://%s%s%s\">..parent..</a></td><br>",ip,atoi(port)==80?"":nport,dir_up(path));
		while((dirent=readdir(dir))!=NULL){
			if(strcmp(path,"/")==0)//website root,no display parent fold
				sprintf(Filename,"/%s",dirent->d_name);
				//create web absolute dir
			else sprintf(Filename,"%s/%s",path,dirent->d_name);
			if(dirent->d_name[0]=='.')
				continue;//隐藏文件不列出,以及..和.
			fprintf(client_sock,"<tr>");
			len=strlen(home_dir)+strlen(Filename)+1;
			realFilename=malloc(len+1);
			bzero(realFilename,len+1);
			sprintf(realFilename,"%s/%s",home_dir,Filename);//主机上的绝对路径
			if(stat(realFilename,&fileinfo)==0){
				 fprintf(client_sock,"<td><a href=\"http://%s%s%s\">%s</a></td><br>",ip,atoi(port)==80?"":nport,Filename,dirent->d_name);
				//p_time=ctime(&info.st_mtime);
				p_passwd=getpwuid(fileinfo.st_uid);//文件拥有者
				p_group=getgrgid(fileinfo.st_gid);//拥有者组

				fprintf(client_sock,"<td>%c</td>",file_type(fileinfo.st_mode));
				fprintf(client_sock,"<td>%s</td>",p_passwd->pw_name);//the file hoster
				fprintf(client_sock,"<td>%s</td><td> %d </td><td>%s</td><br>",p_group->gr_name,(int)fileinfo.st_size,ctime(&fileinfo.st_ctime));
			}
			fprintf(client_sock,"</tr>\n");
			free(realFilename);
		}//while end
		fprintf(client_sock,"</table></center></body></html>");
	}else{//neither file or dir,forbid access
		fprintf(client_sock,"HTTP/1.1 200 OK\r\nServer:Test http server\r\nConnection:close\r\n\r\n<html lang=\"zh-cn\"><html><head><meta charset=\"utf-8\"><title>Permission denied</title></head><body><font size=+4>康哥's HTTP server</font><br><hr width=\"100%%\"><br><table border cols=3 width=\"100%%\">");
		fprintf(client_sock,"</table><font color=\"CC0000\" size=+2> you access resource '%s' forbid to access,communicate with the admintor </font></body></html>",path);
	}
out:
	free(realpath);
	free(nport);
}

char file_type(mode_t st_mode){
	if((st_mode&S_IFMT)==S_IFSOCK) return 's';
	else if((st_mode&S_IFMT)==S_IFLNK) return 'l';
	else if((st_mode&S_IFMT)==S_IFREG) return '-';
	else if((st_mode&S_IFMT)==S_IFBLK) return 'b';
	else if((st_mode&S_IFMT)==S_IFCHR) return 'c';
	else if((st_mode&S_IFMT)==S_IFIFO) return 'p';
	else return 'd';
}
//search the up-path of dirpath
char *dir_up(char *dirpath){
	static char Path[MAXPATH];
	int len;
	strcpy(Path,dirpath);
	len=strlen(Path);
	if(len>1&&Path[len-1]=='/') len--;
	while(Path[len-1]!='/'&&len>1)
		len--;
	Path[len]=0;
	return Path;
}

int get_addr(char *str){
	int inet_sock;
	struct ifreq ifr;
	extern char ip[];
	inet_sock=socket(AF_INET,SOCK_STREAM,0);
	strcpy(ifr.ifr_name,"eth0");//本机接口名
	if(ioctl(inet_sock,SIOCGIFADDR,&ifr)<0){//获取接口信息
		info("ioctl,fail to get eth0 ip");
		exit(-1);
	}
	sprintf(ip,"%s",inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
	return 0;
}
	
char *formatText(char *text){
	int i=0,j=0;
	char *ft=malloc(strlen(text)+2048);
	//it is big bug use sizeof(text),because big file will overflow
	memset(ft,0,strlen(ft));
	//the size of space ,use strcpy +1,not sizeof.
	while(text[i]!='\0'){
		if(text[i]=='<')
			strncpy(ft+j,"&#60;",5);
		else if(text[i]=='>')
			strncpy(ft+j,"&#62;",5);
		else if(text[i]=='&')
			strncpy(ft+j,"&#38;",5);
		else if(text[i]=='\n')
			strncpy(ft+j,"</br>",5);
		else {
			ft[j]=text[i];
			j-=4;
		}
		i++;
		j+=5;
	}
	strcpy(text,ft);
	free(ft);
	ft=NULL;
	return text;
}


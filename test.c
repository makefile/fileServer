#include<stdio.h>
char *postfix(char *file){
	int i=0,lastIndex=0;
	while(file[i]!='\0'){
		if(file[i]=='.') lastIndex=i;
		i++;
	}
	return &file[lastIndex+1];
}
int main(){
	char *str="hello.c";
	printf("postfix:%s\n",postfix(str));
}


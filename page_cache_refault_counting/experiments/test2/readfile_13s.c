#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

int main(){
	int fd=0,n=0,i=0,fd1=0,fd_s=0,wr=0;
	int file_size=10485760;//10MB
	printf("pid=%d\n",getpid());
	//fd=open("./readfile_13s.txt", O_WRONLY, S_IRUSR | S_IWUSR);
	fd=open("./readfile_13s.txt", O_RDONLY, S_IRUSR | S_IWUSR);
	char buff[21];
	char arr[]={'8','9','0','1','2','3','4','5','6','7'};
	char temp[2];
	int pos = 0;
	buff[20]='\0';
	temp[1]='\0';
	i=0;
	while(1){
		i++;
		i = i % 10;
		pos += 2048;
		if(pos>=file_size){
		//	sleep(26);
			pos %= 4096;
			break;
		}
		//temp[0]=arr[i];
		//wr = write(fd , &temp, 1);
		wr = read(fd , buff, 1);
		if(wr<0)
			break;
		wr = lseek(fd , pos , SEEK_SET);
	}
	close(fd);
	return 0;
}

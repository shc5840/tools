#include <stdio.h>      
#include <stdlib.h>    
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>     
#include <termios.h>   
#include <errno.h>     
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>

#define READ_BUFSIZE 	1024
#define WRITE_BUFSIZE	1024
#define DEV_NAME_STR	64

#define DISFORMAT_10	0
#define DISFORMAT_16	1
#define DISFORMAT_CHAR	2

#define RW_FLAG_R		0
#define RW_FLAG_W		1
#define RW_FLAG_RW		2

#define WRITE_FORMAT_10 0
#define WRITE_FORMAT_16 1
#define WRITE_FORMAT_STR 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // static lock
//pthread_mutex_lock(&mutex);
//pthread_mutex_unlock(&mutex);
//pthread_mutex_destroy(&mutex);
//pthread_mutex_trylock(&mutex)

int  g_iThreadFlg;
int  g_iDisFormat;
int  g_iRWFlg;
int  g_iWrite;
int  g_fd;

static int g_readsize = 0;
void read_thread_select(void)
{
	int ret,retval;
	fd_set rfds;
	int RxData=0;

	FD_ZERO(&rfds);
	FD_SET(g_fd,&rfds);
	while(FD_ISSET(g_fd,&rfds))
	{
		if (!g_iThreadFlg) {
			break;
		}

		retval = select(g_fd+1, &rfds, NULL, NULL, NULL);
		if(retval == -1)
		{
			printf("select() failed");
			break;
		}
		else if(retval)
		{
			while(ret = read(g_fd, &RxData, 1)>0)
			{
                g_readsize += ret;
				printf("(%d)0x%x ",g_readsize, RxData);
                fflush(stdout);
			}
		}
		else {
			continue;
		}
	}
}

void read_thread_block(void)
{
	printf("start read thread\n");
	char buff[READ_BUFSIZE];
	int nread=0, icount;
	
	while ( 1 == g_iThreadFlg ) 
	{
		memset(buff, 0x00, READ_BUFSIZE);
		//pthread_mutex_lock(&mutex);
		nread = read(g_fd, buff, READ_BUFSIZE);
		//pthread_mutex_unlock(&mutex);
		if ( nread > 0 ) {
            g_readsize += nread;
			char outbuff[READ_BUFSIZE*4+1] = {0};
			for( icount =0 ; ((icount < nread)&&(icount < READ_BUFSIZE)); icount ++ ){
				switch(g_iDisFormat){
				case DISFORMAT_10:
					sprintf(&outbuff[icount*4],"%03d ",buff[icount]);					
					break;
					
				case DISFORMAT_16:
					sprintf(&outbuff[icount*3],"%02x ",buff[icount]);					
					break;
					
				case DISFORMAT_CHAR:
					sprintf(&outbuff[icount],"%c",buff[icount]);				
					break;				
				}
            }
			printf("(%d)%s",g_readsize, outbuff);
			fflush(stdout);
			//printf("Receive:%s\n",outbuff);
		}
		else{
			//printf("\n");
			usleep(1000);
		}
	}
	
	printf("quit the read thread\n");
}

void help(void)
{
	printf("example: @basename /dev/ttySC1 d r\n");
	printf("	para1:dev name\n");
	printf("	para2:r:read, w:write, rw:read&write\n");
	printf("	para3:display format d:10,x:16,c:char\n");
	printf("	para4:write format d:10,x:16,c:char\n");
}

int main(int argc, char *argv[])
{
	unsigned char szArray[WRITE_BUFSIZE] = {0}; // for send string
	char szQuit[]  = "quit\n";

	g_iThreadFlg=1;
	pthread_t read_thread_id;
	
	if ( argc < 5 ) {
		help();
		return 0;
	}
	char devname[DEV_NAME_STR];	
	snprintf(devname, DEV_NAME_STR, "%s", argv[1]);

	if( 0 == strcmp("r",argv[2]) ) {
		g_iRWFlg = RW_FLAG_R;
	}
	else if( 0 == strcmp("w",argv[2]) ) {
		g_iRWFlg = RW_FLAG_W;
	}
	else if( 0 == strcmp("rw",argv[2]) ) {
		g_iRWFlg = RW_FLAG_RW;
	}
	else{
		printf("error Parameter2 about read write flag\n");
		return -1;
	}
			
	if( 0 == strcmp("d",argv[3]) ) {
		g_iDisFormat = DISFORMAT_10;
	}
	else if( 0 == strcmp("c",argv[3]) ) {
		g_iDisFormat = DISFORMAT_CHAR;
	}
	else if( 0 == strcmp("x",argv[3]) ) {
		g_iDisFormat = DISFORMAT_16;
	}
	else{
		printf("error Parameter3 about display format\n");
		return -1;
	}

	if( 0 == strcmp("d",argv[4]) ) {
		g_iWrite = WRITE_FORMAT_10;
	}
	else if( 0 == strcmp("c",argv[4]) ) {
		g_iWrite = WRITE_FORMAT_STR;
	}
	else if( 0 == strcmp("x",argv[4]) ) {
		g_iWrite = WRITE_FORMAT_16;
	}
	else{
		printf("error Parameter5 about write format\n");
		return -1;
	}
	
	/* open dev */	
	g_fd = open(devname, O_RDWR | O_NONBLOCK);
	if ( g_fd < 0 ) {
       printf("err: can't open dev (%s)!\n",devname);
       return -1;
    }
    	
	printf("Open dev <%s> success \n", devname);
			
	// create the read thread
	if ((RW_FLAG_R == g_iRWFlg) || (RW_FLAG_RW == g_iRWFlg)) {
        while(1){
            printf("input ss then enter start read\n");
            fgets( szArray, WRITE_BUFSIZE, stdin );
		    if ( 0 == memcmp( szArray, "ss\n",  strlen("ss\n") ) ){
			    break;
		    }
        }

		if ( 0 != pthread_create(&read_thread_id, NULL, (void*)read_thread_block, NULL) ) {
			printf("creat the read thread failed\n");
		}
	}
	
	while(1) {
		memset(szArray, 0x00, sizeof(szArray));
		
		if (RW_FLAG_R == g_iRWFlg) {
			char* ret = fgets( szArray, WRITE_BUFSIZE, stdin );

			if ( NULL == ret ){
				g_iThreadFlg = 0;
				break;
			}
			
			if ( 0 == memcmp( szArray, szQuit,  strlen(szQuit) ) ){
				g_iThreadFlg = 0;
				break;
			}

			continue;
		}

		int icount=0;
		//scanf("%*[^0-9a-fA-F\n]");
		if(g_iWrite == WRITE_FORMAT_STR){
			do{
				scanf("%c",&szArray[icount]);
			}while(szArray[icount++]!='\n');

			if ( 0 == memcmp( szArray, szQuit,  strlen(szQuit) ) ){
				g_iThreadFlg = 0;
				break;
			}
			icount--;
			szArray[icount]=0;
			printf("send:%s, count=%d\n",szArray, icount);
		}
		
		if(g_iWrite == WRITE_FORMAT_10){
			int j;
			scanf("%*[^0-9]");
			while(scanf("%d",&szArray[icount]) != 0)icount++;
				
			printf("send:");
			for(j=0; j<icount; j++)
				printf("%d ",szArray[j]);
			printf(", count=%d\n",icount);
		}

		if(g_iWrite == WRITE_FORMAT_16){
			int j;
			scanf("%*[^0-9a-fA-F]");
			while(scanf("%x",&szArray[icount]) != 0)icount++;

			printf("send:");
			for(j=0; j<icount; j++)
				printf("0x%x ",szArray[j]);
			printf(", count=%d\n",icount);
		}

		fflush(stdin);

		//pthread_mutex_lock(&mutex);
		write(g_fd, szArray, icount);
		//pthread_mutex_unlock(&mutex);
	}
	
	if ((RW_FLAG_R == g_iRWFlg) || (RW_FLAG_RW == g_iRWFlg)) {
		pthread_join(read_thread_id,NULL); //wait for quit thread
	}
	pthread_mutex_destroy(&mutex);
	printf("wait close g_fd\n");
    close(g_fd);
	printf("close g_fd ok\n");
	
	return 0;
}


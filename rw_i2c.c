/*=============================================================================
    (c)  PIONEER CORPORATION  2012
    25-1 Nishi-machi Yamada Kawagoe-shi Saitama-ken 350-8555 Japan
    All Rights Reserved.

=============================================================================*/

#include <stdio.h>  
#include <linux/types.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/ioctl.h>  
#include <errno.h>  
#include <assert.h>  
#include <string.h>  
#include <linux/i2c.h>  
#include <linux/i2c-dev.h>  


static int iic_read(unsigned int fd, unsigned int slave_address, unsigned int len, unsigned char *data, unsigned int data_len)
{
    struct i2c_rdwr_ioctl_data iic_data;  
	int ret = 0;

	ioctl(fd,I2C_SLAVE,slave_address);
    ioctl(fd, I2C_TIMEOUT, 2); 
    ioctl(fd, I2C_RETRIES, 1);  
    ioctl(fd, I2C_TENBIT,0);

    /*read data from i2c device*/  
    iic_data.nmsgs = 2;
	iic_data.msgs = (struct i2c_msg *)malloc(iic_data.nmsgs * sizeof(struct i2c_msg)); 
	if (!iic_data.msgs)
	{  
        printf("Memory alloc error.\n");   
        return -12;  
    }  
    iic_data.msgs[0].len = data_len; 
    iic_data.msgs[0].addr = slave_address;  
    iic_data.msgs[0].flags = 0;
    iic_data.msgs[0].buf = (unsigned char*)malloc(data_len); 
	int j = 0;
	for ( j = 0; j < data_len; ++j ) 
	{
		iic_data.msgs[0].buf[j] = data[j];
	}
 //   *r = ( ( reg_address & 0x00FF ) << 8 ) | ( ( reg_address & 0xFF00 ) >> 8 );
    
 //  (((__u16)(x) & (__u16)0x00ffU) << 8) |			\
	//(((__u16)(x) & (__u16)0xff00U) >> 8))
    
//	unsigned short r = cpu_to_be16(reg_address);
//	memcpy(&iic_data.msgs[0].buf[0],2,r);
    //iic_data.msgs[0].buf[0] = reg_address;  
          
    iic_data.msgs[1].len = len;  
    iic_data.msgs[1].addr = slave_address;  
    iic_data.msgs[1].flags = I2C_M_RD;
    iic_data.msgs[1].buf = (unsigned char*)malloc(len);  
    iic_data.msgs[1].buf[0] = 0;
    ret = ioctl (fd, I2C_RDWR, (unsigned long)&iic_data);
    if (ret < 0)
	{  
        printf ("ioctl read error.\n"); 
	return -5; 
    }
	else
	{
    	printf("The value read from i2c device ID(0x%X) is: \n",slave_address);
		int i = 0;
		for( i = 0; i < len; i++ )
		{
			printf(" %02x",iic_data.msgs[1].buf[i]); 
		}
		printf("\r\n");
	}
	free(iic_data.msgs[0].buf);	
	free(iic_data.msgs[1].buf);	
	free(iic_data.msgs);	

    return 0;
}

static int iic_write(unsigned int fd, unsigned int slave_address, unsigned char * data, unsigned int data_len)
{
    struct i2c_rdwr_ioctl_data iic_data; 
	int ret = 0;
	ioctl(fd,I2C_SLAVE,slave_address);
    ioctl(fd, I2C_TIMEOUT, 2); 
    ioctl(fd, I2C_RETRIES, 1);  
    ioctl(fd,I2C_TENBIT,0);

	iic_data.nmsgs = 1;
    iic_data.msgs = (struct i2c_msg *)malloc(iic_data.nmsgs * sizeof(struct i2c_msg));  
	if (!iic_data.msgs)
	{  
        printf("Memory alloc error.\n");   
        return -12;  
    }  
    iic_data.msgs[0].len = data_len;
    iic_data.msgs[0].addr = slave_address;  
    iic_data.msgs[0].flags = 0;  
    iic_data.msgs[0].buf = (unsigned char*)malloc(data_len);
	int j = 0;
	for ( j = 0; j < data_len; ++j ) 
	{
		iic_data.msgs[0].buf[j] = data[j];
	}  
  //  iic_data.msgs[0].buf[0] = reg_address;
/*	unsigned short * r = iic_data.msgs[0].buf;
    *r = ( ( reg_address & 0x00FF ) << 8 ) | ( ( reg_address & 0xFF00 ) >> 8 );
	memcpy(&iic_data.msgs[0].buf[2], value, len);
	printf("iic_data.msgs[0].buf[2] = %x\n\r",iic_data.msgs[0].buf[2]);
	printf("iic_data.msgs[0].buf[3] = %x\n\r",iic_data.msgs[0].buf[3]);
	int i = 0;*/
/*	int j = 0;
	for(i = 0, j = len - 1; i < len && j >= 0; i++, j--)
	{
		iic_data.msgs[0].buf[i+2] = value[j];
		printf("%x",value[j]);
	}*/
/*	for(i = 0; i < len; i++)
	{
		printf("%x",value[i]);
	}*/


    ret = ioctl (fd, I2C_RDWR, (unsigned long)&iic_data);
    if (ret <0)
	{  
        printf ("ioctl write error\n"); 
	return -5; 
    } 
	else
	{
		printf(" Write to i2c device ID(0x%X) success!\n",slave_address);  
	} 
	
	free(iic_data.msgs[0].buf);	
	free(iic_data.msgs);	
    return 0;
}

int main(int argc, char *argv[])
{
	unsigned int fd;
    unsigned int slave_address, len, data_len;
	unsigned char  * data;  

	char rw; //0 - Read ,1 - Write
    if (argc < 6){  
        printf("Usage:\n%s /dev/i2c-x r slave_address len data1 data2 ... \n Or\n%s /dev/i2c-x w slave_address data1 data2 ...\n",argv[0],argv[0]);  
        return -125;  
    }  
	sscanf(argv[2], "%c", &rw);  
	sscanf(argv[3], "%x", &slave_address);  
	//sscanf(argv[4], "%x", &reg_address);  
	//sscanf(argv[5], "%d", &len); 

    fd = open(argv[1], O_RDWR);  
   
    if (!fd){  
        printf("Error on opening the device file.\n");  
        return -2;  
    }  
	if('r' == rw)
	{ //read
		sscanf(argv[4], "%x", &len); 
		data_len = argc - 5;
		data = (unsigned char *)malloc(data_len); 
		int i = 0;
		for(i = 0; i < data_len; i++)
		{
			sscanf(argv[5+i], "%x", &data[i]);  
			
		}
		iic_read(fd,slave_address,len,data,data_len);
		free(data);
	}
	else
	{
/*		if (argc < 6 + len ){  
 			printf("Usage:\n%s /dev/i2c-x r slave_address reg_addr len \n Usage:\n%s /dev/i2c-x w slave_address reg_addr len value1 value2\n",argv[0],argv[0]);  
        	return 0;  
		}*/
		data_len = argc - 4;
		data = (unsigned char *)malloc(data_len); 
		int i = 0;
		for(i = 0; i < data_len; i++)
		{
			sscanf(argv[4+i], "%x", &data[i]);  
			
		}
		iic_write(fd,slave_address,data,data_len);
		free(data);
    }
 	close(fd);  
	return 0;
}



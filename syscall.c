#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/file.h>
#include <linux/net.h> 
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>


#define LICENSE "GPL" 
#define DESCRIPTION "CSc239 Presentation" 
#define AUTHOR "KARAN MITRA" 
#define PROC_FILE "hist_file"
#define PROC_NET "hist_net"
#define PROC_MAX_SIZE 4096
MODULE_LICENSE(LICENSE); 
MODULE_DESCRIPTION(DESCRIPTION); 
MODULE_AUTHOR(AUTHOR); 

const char *filesBlock[];

const char *netBlock[];


char *read_buff, *net_buff;
//char **file = NULL;
char USER_NAME[11]="USERNAMEREP\0";
char USER_TIME[11]="###:##:###";
char log_filename[11]="##_##_####"; 
int len,temp;
int print = 0;
char *msg;
static unsigned long buffer_size = 0;
static unsigned long writeIndex = 0;
static unsigned long net_writeIndex = 0;
//static int read_index;

unsigned long *sys_call_table; 
char *proc_data;

char *net_proc_data;
/****FUNCTION PROTOTYPES*******/

int get_username(char *); 
void print_time(char char_time[]);

asmlinkage int (*original_close)(unsigned int);
asmlinkage long (*original_sys_open)(const char *,int,int); 
asmlinkage size_t (*original_read)(int, char *, size_t); 
asmlinkage int (*original_write)(unsigned int, const char __user *, size_t); 

asmlinkage long (*original_connect)(int, struct sockaddr __user *, int);
asmlinkage long new_connect(int fd, struct sockaddr __user *buff1, int flag);

asmlinkage long (*original_accept)(int, struct sockaddr __user *, int __user *);
asmlinkage long new_accept(int fd, struct sockaddr __user *buff1, int __user *buff2) ;

asmlinkage long (*original_sendto)(int, void __user *, size_t, unsigned,struct sockaddr __user *, int);
asmlinkage long new_sendto(int fd, void __user *buff1, size_t len, unsigned flags, struct sockaddr __user *addr, int addr_len);

asmlinkage long (*original_recvfrom)(int, void __user *, size_t, unsigned, struct sockaddr __user *, int __user *); 
asmlinkage long new_recvfrom(int fd, void __user *buff1, size_t len, unsigned flags, struct sockaddr __user *ar, int __user *buff2);

asmlinkage int (*original_getsockname)(int,struct sockaddr __user *,int *);
asmlinkage int (*original_getpeername)(int,struct sockaddr __user *,int *);

char *inet_ntoa(struct in_addr inn);
short unsigned int my_ntoh(short unsigned int src_port);

ssize_t write_proc(const char *buff, size_t len);
ssize_t write_net(const char *buff, size_t len);

ssize_t read_net(struct file *filp,char *buffer,size_t count,loff_t *offp );

asmlinkage long our_sys_open(const char *filename,int flags,int mode) { 

	int ret, fd;
//	printk(KERN_INFO "Open request at - %s \n", filename);

//	printk(KERN_INFO "file - %s and proc data size = %zu",file, sizeof(proc_data));


//	if (strstr(filesBlock,filename) != NULL && *filename != '.' && *filename != '/') {
//	if (strncmp(file, filename, strlen(filename)) == 0) {

/*	int len2 = sizeof(filename)/sizeof(filesBlock[0]);
	printk(KERN_INFO "length = %d", len2);
	int i;

	for(i = 0; i < len2; ++i) {
	
		printk(KERN_INFO "i = %d and string = %s", i, filesBlock);
		if(strcmp(filesBlock[i],  filename)) {
			printk(KERN_INFO "\nfound");
		}


	}    */
	

	if (strstr(read_buff,filename) != NULL && *filename != '.' && *filename != '/') {
//	if (strncmp(file, filename, strlen(filename)) == 0) {
		ret=get_username(USER_NAME);
		if(ret < 0) {
			printk(KERN_ALERT "\n 8===error in get_username");
//			return -1;
		}
		else { 
			char tmp_buff[12];
			print_time(USER_TIME); 
//			printk(KERN_EMERG "\n time - %s", USER_TIME);
			if(!proc_data)
				printk(KERN_EMERG "something went wrong");
			strcat(proc_data, "Access try of file ");
			strcat(proc_data, filename);
			strcat(proc_data, " by ");
			strcat(proc_data, USER_NAME);
			strcat(proc_data, " with process ");
			snprintf(tmp_buff,12,"%u",current->pid);
			strcat(proc_data, tmp_buff);
			strcat(proc_data, " called ");
			strcat(proc_data, current->comm);
			strcat(proc_data, " at time ");
			strcat(proc_data, USER_TIME);
			strcat(proc_data, "\n");
			
			len = sizeof(proc_data);
	
/*			mm_segment_t old_fs = get_fs();
	                set_fs(KERNEL_DS);

			fd = our_sys_open("/proc/hist_file", O_RDWR ,0700);
	                printk(KERN_EMERG "proc_fd = %d",fd);
			original_write(fd, proc_data, len); 
			set_fs(old_fs);  */

			write_proc(proc_data, strlen(proc_data));

			printk(KERN_INFO "badtry - %s", proc_data);
			return -EACCES;
		}
	}
//	printk(KERN_INFO "Allowing access");
        return original_sys_open(filename,flags,mode); /* call the original function */ 
} 


ssize_t read_proc(struct file *filp,char *buffer,size_t count,loff_t *offp ) 
{
	static int finished = 0;

	printk(KERN_INFO "In read with finished = %d", finished);

	if(finished == 1) {
		printk(KERN_INFO "finished");
		finished = 0;
		return 0;
	}  


	finished = 1;	
	if(copy_to_user(buffer, proc_data, writeIndex)) {
		printk(KERN_INFO "Error in reading");
		return -EFAULT;
	}

//	printk(KERN_INFO "file state after reading- %s", filesBlock);
	return writeIndex;

}

ssize_t write_proc(const char *buff, size_t len) {

	writeIndex = 0;
	printk(KERN_INFO "\n In proc write");
	int capacity = (PROC_MAX_SIZE - writeIndex) + 1;

	if(len > capacity) {

		printk(KERN_INFO "\nNo space in file");
		return -1;

	}

	printk(KERN_INFO "\n Writing %s to proc at index = %lu", proc_data, writeIndex);

	strcpy(&proc_data[writeIndex], buff);
	writeIndex += len;
	proc_data[writeIndex - 1] = '\0';
	return len;
}

struct file_operations proc_fops = {
        read: read_proc,
        write: write_proc
};


struct file_operations proc_nops = {
	read:  read_net,
	write: write_net
};


ssize_t read_net(struct file *filp,char *buffer,size_t count,loff_t *offp ) 
{
        static int finished = 0;

        printk(KERN_INFO "In read with finished = %d", finished);

        if(finished == 1) {
                printk(KERN_INFO "finished");
                finished = 0;
                return 0;
        }  


        finished = 1;   
        if(copy_to_user(buffer, net_proc_data, net_writeIndex)) {
                printk(KERN_INFO "Error in reading");
                return -EFAULT;
        }

//      printk(KERN_INFO "file state after reading- %s", filesBlock);
        return net_writeIndex;

}

ssize_t write_net(const char *buff, size_t len) {

        net_writeIndex = 0;
        printk(KERN_INFO "\n In proc write");
        int capacity = (PROC_MAX_SIZE - net_writeIndex) + 1;

        if(len > capacity) {

                printk(KERN_INFO "\nNo space in file");
                return -1;

        }

        printk(KERN_INFO "\n Writing %s to proc at index = %lu", net_proc_data, net_writeIndex);

        strcpy(&net_proc_data[net_writeIndex], buff);
        net_writeIndex += len;
        net_proc_data[net_writeIndex - 1] = '\0';
        return len;
}

int my_init(void) 
           {          
		int fd, netfd, ret, my_i; 
	   	printk(KERN_INFO "I am Hacking the system call\n"); 
		write_cr0 (read_cr0 () & (~ 0x10000));
                sys_call_table = (unsigned long *) 0xffffffff81a00200;           /*assign the system  call  table  base  address  to  sys_call_table*/ 

                original_sys_open = sys_call_table[__NR_open];  /*  Save  the  original  sys_open address , so we can call the original sys_open function */ 
		sys_call_table[__NR_open]  =  (void  *) our_sys_open;  /*  change  the  system  call base table entry of sys_open to our_sys_open function */ 

		original_read = sys_call_table[__NR_read];
		original_close=(void *)sys_call_table[__NR_close]; 
		original_write= (void *)sys_call_table[__NR_write]; 

		original_connect=(void *)sys_call_table[__NR_connect];
		original_accept=(void *)sys_call_table[__NR_accept];
		original_sendto=(void *)sys_call_table[__NR_sendto];
		original_recvfrom=(void *)sys_call_table[__NR_recvfrom];

		original_getsockname=(void *)sys_call_table[__NR_getsockname];
		original_getpeername=(void *)sys_call_table[__NR_getpeername];

		sys_call_table[__NR_connect] = (void *) new_connect;
		sys_call_table[__NR_accept] = (void *) new_accept;
		sys_call_table[__NR_sendto] = (void *) new_sendto;
		sys_call_table[__NR_recvfrom] = (void *) new_recvfrom;

		write_cr0 (read_cr0 () | 0x10000);                

	
		mm_segment_t old_fs = get_fs();
		set_fs(KERNEL_DS);

		proc_create(PROC_FILE,0644,NULL,&proc_fops);
		proc_create(PROC_NET,0644,NULL,&proc_nops);
		
                proc_data = (char *)kmalloc(PROC_MAX_SIZE,GFP_KERNEL);
		net_proc_data = (char *)kmalloc(PROC_MAX_SIZE,GFP_KERNEL);
/*		fd = our_sys_open("/home/karanm1992/block.txt", O_RDONLY,0700);
		printk(KERN_INFO "fd = %d",fd);
		original_read(fd, filesBlock, 64);
		printk("<1>the file content is:\n");
           		printk("<1>%s\n",filesBlock);	 */


		read_buff = (char *)kmalloc(2024,GFP_ATOMIC);
		fd = original_sys_open("/home/karanm1992/block_files.conf", O_RDONLY|O_LARGEFILE, 0700);
		ret=original_read(fd,read_buff,2024);

		for(my_i=0;my_i<ret;my_i++){
                if(read_buff[my_i] == '\0')
                         read_buff[my_i] = ' ';
                }
                read_buff[ret-1] = '\0';

		printk("file contents are - %s", read_buff);


		ret = 0;
		net_buff = (char *)kmalloc(2024,GFP_ATOMIC);
                netfd = original_sys_open("/home/karanm1992/block_net.conf", O_RDONLY|O_LARGEFILE, 0700);
		ret=original_read(netfd,net_buff,2024);

		printk(KERN_INFO "ret for net is %d", ret);
		for(my_i=0;my_i<ret;my_i++){
                if(net_buff[my_i] == '\0')
                         net_buff[my_i] = ' ';
                }
                net_buff[ret-1] = '\0';

                printk("net file contents are - %s", net_buff);

/*		fdnet = our_sys_open("/home/karanm1992/netblock.txt", O_RDONLY,0700);
                printk(KERN_INFO "netfd = %d",fdnet);

                original_read(fdnet, netBlock, 32);
                printk("<2>\nnet file content is:\n");
                printk("<2>%s", netBlock);   */


//		memset(proc_data, 0, 4096);
//		write_index = 0;
//	        read_index = 0;

		set_fs(old_fs);

		return 0;
//			while ((c = getc(file)) != EOF)
//		        putchar(c);
		
           }           

asmlinkage long new_connect(int fd, struct sockaddr __user *buff1, int flag) {
	
	int ret, ret1, ret2,fc;
	struct sockaddr_in getsock, getpeer;
	struct sockaddr_in *getsock_p, *getpeer_p;
	int socklen;
	char netinfo_buff[200], path[120];
	char buff[100];

	socklen=sizeof(getsock);
	mm_segment_t old_fs=get_fs();
	set_fs(KERNEL_DS);
	
	ret1=original_getsockname(fd,(struct sockaddr *)&getsock,&socklen);
	getsock_p=&getsock;
	ret2=original_getpeername(fd,(struct sockaddr *)&getpeer,&socklen);
	getpeer_p=&getpeer;
	
	set_fs(old_fs);
	
//	printk("\nret1 is %d %d",ret1, ret2);

	if(getsock.sin_family==2)
	{	
		print_time(USER_TIME);
		strcpy(netinfo_buff,USER_TIME+1);
//		get_username(USER_NAME);
		strcat(netinfo_buff,USER_NAME);
		snprintf(buff,9,"#%s","Connect");
		strcat(netinfo_buff,buff);
		snprintf(buff,18, "#%s",inet_ntoa(getsock.sin_addr));
		strcat(netinfo_buff,buff);
		snprintf(buff,10,"#%u",my_ntoh(getsock.sin_port));
		strcat(netinfo_buff,buff);
		snprintf(buff,18,"#%s",inet_ntoa(getpeer.sin_addr));
		strcat(netinfo_buff,buff);
		snprintf(buff,10,"#%u\n",my_ntoh(getpeer.sin_port));
		strcat(netinfo_buff,buff);
		printk(KERN_INFO "\n netbuff - %s", netinfo_buff);

		printk(KERN_INFO "\n connectgrep %s in %s", net_buff, netinfo_buff);

		if (strstr(netinfo_buff,net_buff) != NULL && net_buff[-1] == '\0') {
				char tmp_buff[12];
                                print_time(USER_TIME); 
                                get_username(USER_NAME);
        //                      printk(KERN_EMERG "\n time - %s", USER_TIME);
                                if(!net_proc_data)
                                        printk(KERN_EMERG "something went wrong");
                                strcat(net_proc_data, "Connect to try of IP ");
                                strcat(net_proc_data, net_buff);
                                strcat(net_proc_data, " by ");
                                strcat(net_proc_data, USER_NAME);
                                strcat(net_proc_data, " with process ");
                                snprintf(tmp_buff,12,"%u",current->pid);
                                strcat(net_proc_data, tmp_buff);
                                strcat(net_proc_data, " called ");
                                strcat(net_proc_data, current->comm);
                                strcat(net_proc_data, " at time ");
                                strcat(net_proc_data, USER_TIME);
                                strcat(net_proc_data, "\n");

                                printk(KERN_INFO "\nAccess try - %s", net_proc_data);
                                write_net(net_proc_data, strlen(net_proc_data));

			printk(KERN_INFO "\npinka - found");
			return -ECONNREFUSED;
		}
	}
	
//	printk(KERN_INFO " pasta - in new connect with fd = %d", fd);
	return original_connect(fd, buff1, flag);
}	

asmlinkage long new_accept(int fd, struct sockaddr __user *buff1, int __user *buff2) {
//	 printk(KERN_INFO " pasta - in new accept with fd = %d", fd);

	int ret, ret1, ret2,fc;
	struct sockaddr_in getsock, getpeer;
	struct sockaddr_in *getsock_p, *getpeer_p;
	int socklen;
	char netinfo_buff[200], path[120];
	char buff[100];
	socklen=sizeof(getsock);
	mm_segment_t old_fs=get_fs();
	set_fs(KERNEL_DS);
	ret1=original_getsockname(fd,(struct sockaddr *)&getsock,&socklen);
	getsock_p=&getsock;
	ret2=original_getpeername(fd,(struct sockaddr *)&getpeer,&socklen);getpeer_p=&getpeer;
	set_fs(old_fs);
//	printk("\nret1 is %d %d",ret1, ret2);

	if(getsock.sin_family==2)
	{
		print_time(USER_TIME);
		strcpy(netinfo_buff,USER_TIME+1);
//		ret=get_username(USER_NAME);

		strcat(netinfo_buff,USER_NAME);
		snprintf(buff,8,"#%s","Accept");
		strcat(netinfo_buff,buff);
		snprintf(buff,18, "#%s",inet_ntoa(getsock.sin_addr));
		strcat(netinfo_buff,buff);
		snprintf(buff,10,"#%u",my_ntoh(getsock.sin_port));
		strcat(netinfo_buff,buff);
		snprintf(buff,18,"#%s",inet_ntoa(getpeer.sin_addr));
		strcat(netinfo_buff,buff);
		snprintf(buff,10,"#%u\n",my_ntoh(getpeer.sin_port));
		strcat(netinfo_buff,buff);

//		printk(KERN_INFO "\n netbuff - %s", netinfo_buff);

                printk(KERN_INFO "\n acceptgrep %s in %s", net_buff, netinfo_buff);

                if (strstr(netinfo_buff,net_buff) != NULL) {
                        printk(KERN_INFO "\npinka - found");

			 char tmp_buff[12];
                        print_time(USER_TIME); 
//                      printk(KERN_EMERG "\n time - %s", USER_TIME);
                        if(!net_proc_data)
                                printk(KERN_EMERG "something went wrong");
                        strcat(net_proc_data, "Accept try on IP");
                        strcat(net_proc_data, net_buff);
                        strcat(net_proc_data, " by ");
                        strcat(net_proc_data, USER_NAME);
                        strcat(net_proc_data, " with process ");
                        snprintf(tmp_buff,12,"%u",current->pid);
                        strcat(net_proc_data, tmp_buff);
                        strcat(net_proc_data, " called ");
                        strcat(net_proc_data, current->comm);
                        strcat(net_proc_data, " at time ");
                        strcat(net_proc_data, USER_TIME);
                        strcat(net_proc_data, "\n");

			write_net(net_proc_data, strlen(net_proc_data));
			return -ECONNREFUSED;
                }

	}
        return original_accept(fd, buff1, buff2);
}

asmlinkage long new_sendto(int fd, void __user *buff1, size_t len, unsigned flags, struct sockaddr __user *addr, int
addr_len) {
//	printk(KERN_INFO " pasta - in new sendto with fd = %d", fd);

	int ret, ret1, ret2,fc;
	struct sockaddr_in getsock, getpeer;
	struct sockaddr_in *getsock_p, *getpeer_p;
	int socklen;
	char netinfo_buff[200], path[120];
	char buff[100];
	socklen=sizeof(getsock);
	mm_segment_t old_fs=get_fs();
	set_fs(KERNEL_DS);
	ret1=original_getsockname(fd,(struct sockaddr *)&getsock,&socklen);
	getsock_p=&getsock;ret2=original_getpeername(fd,(struct sockaddr *)&getpeer,&socklen);
	getpeer_p=&getpeer;
	set_fs(old_fs);
//	printk("\nret1 is %d %d",ret1, ret2);
	if(getsock.sin_family==2)
	{
		print_time(USER_TIME);
		strcpy(netinfo_buff,USER_TIME+1);
//		ret=get_username(USER_NAME);

			strcat(netinfo_buff,USER_NAME);
			snprintf(buff,8,"#%s","SEND");
			strcat(netinfo_buff,buff);
			snprintf(buff,18, "#%s",inet_ntoa(getsock.sin_addr));
			strcat(netinfo_buff,buff);
			snprintf(buff,10,"#%u",my_ntoh(getsock.sin_port));
			strcat(netinfo_buff,buff);
			snprintf(buff,18,"#%s",inet_ntoa(getpeer.sin_addr));
			strcat(netinfo_buff,buff);
			snprintf(buff,10,"#%u\n",my_ntoh(getpeer.sin_port));
			strcat(netinfo_buff,buff);

			printk(KERN_INFO "\n sendtogrep %s in %s", net_buff, netinfo_buff);

	                if (strstr(netinfo_buff,net_buff) != NULL) {
	                        printk(KERN_INFO "\npinka - found");

				char tmp_buff[12];
	                        print_time(USER_TIME); 
				get_username(USER_NAME);
	//                      printk(KERN_EMERG "\n time - %s", USER_TIME);
	                        if(!net_proc_data)
	                                printk(KERN_EMERG "something went wrong");
	                        strcat(net_proc_data, "UDP Send to try of IP ");
	                        strcat(net_proc_data, net_buff);
	                        strcat(net_proc_data, " by ");
	                        strcat(net_proc_data, USER_NAME);
	                        strcat(net_proc_data, " with process ");
	                        snprintf(tmp_buff,12,"%u",current->pid);
	                        strcat(net_proc_data, tmp_buff);
	                        strcat(net_proc_data, " called ");
	                        strcat(net_proc_data, current->comm);
	                        strcat(net_proc_data, " at time ");
	                        strcat(net_proc_data, USER_TIME);
	                        strcat(net_proc_data, "\n");

				printk(KERN_INFO "\nAccess try - %s", net_proc_data);
				write_net(net_proc_data, strlen(net_proc_data));
				return -ECONNREFUSED;

	                }

	}
	
	return original_sendto(fd, buff1, len, flags, addr, addr_len);

}

asmlinkage long new_recvfrom(int fd, void __user *buff1, size_t len, unsigned flags, struct sockaddr __user *ar, int
__user *buff2) {
//	printk(KERN_INFO " pasta - in new recvfrom with fd = %d", fd);


	int ret, ret1, ret2,fc;
	struct sockaddr_in getsock, getpeer;
	struct sockaddr_in *getsock_p, *getpeer_p;
	int socklen;
	char netinfo_buff[200], path[120];
	char buff[100];
	socklen=sizeof(getsock);
	mm_segment_t old_fs=get_fs();
	set_fs(KERNEL_DS);
	ret1=original_getsockname(fd,(struct sockaddr *)&getsock,&socklen);
	getsock_p=&getsock;ret2=original_getpeername(fd,(struct sockaddr *)&getpeer,&socklen);
	getpeer_p=&getpeer;
	set_fs(old_fs);
//	printk("\nret1 is %d %d",ret1, ret2);

	if(getsock.sin_family==2)
	{
		print_time(USER_TIME);
//		get_username(USER_NAME);
		strcpy(netinfo_buff,USER_TIME+1);
//		ret=get_username(USER_NAME);
		strcat(netinfo_buff,USER_NAME);
		snprintf(buff,9,"#%s","RECEIVE");
		strcat(netinfo_buff,buff);
		snprintf(buff,18, "#%s",inet_ntoa(getsock.sin_addr));
		strcat(netinfo_buff,buff);
		snprintf(buff,10,"#%u",my_ntoh(getsock.sin_port));
		strcat(netinfo_buff,buff);
		snprintf(buff,18,"#%s",inet_ntoa(getpeer.sin_addr));
		strcat(netinfo_buff,buff);
		snprintf(buff,10,"#%u\n",my_ntoh(getpeer.sin_port));
		strcat(netinfo_buff,buff);

		printk(KERN_INFO "\n recvfromgrep %s in %s", net_buff, netinfo_buff);

                if (strstr(netinfo_buff,net_buff) != NULL) {
				char tmp_buff[12];
                                print_time(USER_TIME); 
                                get_username(USER_NAME);
        //                      printk(KERN_EMERG "\n time - %s", USER_TIME);
                                if(!net_proc_data)
                                        printk(KERN_EMERG "something went wrong");
                                strcat(net_proc_data, "UDP Receive to try of IP ");
                                strcat(net_proc_data, net_buff);
                                strcat(net_proc_data, " by ");
                                strcat(net_proc_data, USER_NAME);
                                strcat(net_proc_data, " with process ");
                                snprintf(tmp_buff,12,"%u",current->pid);
                                strcat(net_proc_data, tmp_buff);
                                strcat(net_proc_data, " called ");
                                strcat(net_proc_data, current->comm);
                                strcat(net_proc_data, " at time ");
                                strcat(net_proc_data, USER_TIME);
                                strcat(net_proc_data, "\n");

                                printk(KERN_INFO "\nAccess try - %s", net_proc_data);
                                write_net(net_proc_data, strlen(net_proc_data));

	                printk(KERN_INFO "\npinka - found");
//			return -ECONNREFUSED;
                }

	}
        return original_recvfrom(fd, buff1, len, flags, ar, buff2);
}

void my_exit(void) 
           {           
                      if(sys_call_table[__NR_open] != our_sys_open) /* This check is must because if this is the case then somebody else also played with sys_call_table's sys_open */ 
		   { 
			printk(KERN_ALERT "Somebody else also played with system call table\n"); 
                        printk(KERN_ALERT "The system may be left in unstable state\n"); 
		   } 
			write_cr0 (read_cr0 () & (~ 0x10000));

                        sys_call_table[__NR_open] = (void *)original_sys_open; 
			sys_call_table[__NR_connect] = (void *) original_connect;
	                sys_call_table[__NR_accept] = (void *) original_accept;
        	        sys_call_table[__NR_sendto] = (void *) original_sendto;
                	sys_call_table[__NR_recvfrom] = (void *) original_recvfrom;


			remove_proc_entry(PROC_FILE,NULL);
			remove_proc_entry(PROC_NET,NULL);

			write_cr0 (read_cr0 () | 0x10000);
			kfree(proc_data);
			kfree(net_proc_data);
 /*  
assign the original ad
dress for system integrity */ 
			printk(KERN_INFO "Leaving the kernel\n"); 
           }           

int get_username(char *name)
{
	char *read_buff2,*path0,*tk0,*tk10;
	char tmp_buff0[12];
	int fd0,ret0,my_i0,error=0;
	mm_segment_t old_fs_username;
	read_buff2 = (char *)kmalloc(2024,GFP_ATOMIC);
	
	if(!read_buff2){
		 printk(KERN_ALERT "\n1=== kmalloc error");
		 return -1;
	} 
	path0 = (char *)kmalloc(120,GFP_ATOMIC);

	if(!path0){
		 printk(KERN_ALERT "\n2=== kmalloc error for path");
		 return -1;
	}

	strcpy(path0,"/proc/");
	snprintf(tmp_buff0,12,"%u",current->pid);
	strcat(path0,tmp_buff0);
	strcat(path0,"/environ");
	old_fs_username = get_fs();
	set_fs(KERNEL_DS);
	fd0 = original_sys_open(path0, O_RDONLY|O_LARGEFILE, 0700); // Original Stolenaddress of sys_open system call
	 if(fd0 < 0){
		 printk(KERN_ALERT "\n3=== error in sys_open in get_username function");
		 error = -1;
		 goto my_error;
	 }
	 else{
		 if((ret0=original_read(fd0,read_buff2,2024)) < 0){
			 printk(KERN_ALERT "\n 4===Error in reading in get_username function");
			 error = -1;
			 goto my_error;
		 }
	 else{
		 for(my_i0=0;my_i0<ret0;my_i0++){
		 if(read_buff2[my_i0] == '\0')
			 read_buff2[my_i0] = ' ';
		 }
		 read_buff2[ret0-1] = '\0';
		 printk(KERN_INFO "\n substring in %s", read_buff2);
		 tk0 = strstr(read_buff2,"USER=");
		 if(!tk0){
			 printk(KERN_ALERT "\n 5===Error in strstr");
			 error = -1;
			 goto my_error;
		 }
		 tk10 = strsep(&tk0," ");
		 tk10 = tk10+5;
		 strncpy(name,tk10,11);
	 }

	 original_close(fd0);
	 }

my_error:
 set_fs(old_fs_username);
 kfree(read_buff2);
 kfree(path0);
 return error;
} 

void print_time(char char_time[])
{
 struct timeval my_tv;
 int sec, hr, min, tmp1, tmp2;
 int days,years,days_past_currentyear;
 int i=0,month=0,date=0;
 unsigned long get_time;
 char_time[11]="#00:00:00#";

 do_gettimeofday(&my_tv); // Get System Time From Kernel Mode
 get_time = my_tv.tv_sec; // Fetch System time in Seconds
// printk(KERN_ALERT "\n %ld",get_time);
 get_time = get_time + 43200;
 sec = get_time % 60; // Convert into Seconds
 tmp1 = get_time / 60;
 min = tmp1 % 60; // Convert into Minutes
 tmp2 = tmp1 / 60;
 hr = (tmp2+4) % 24; // Convert into Hours
 hr=hr+1;
 char_time[1]=(hr/10)+48; // Convert into Char from Int
 char_time[2]=(hr%10)+48;
 char_time[4]=(min/10)+48;
 char_time[5]=(min%10)+48;
 char_time[7]=(sec/10)+48;
 char_time[8]=(sec%10)+48;
 char_time[10]='\0';
 /* calculating date from time in seconds */
 days = (tmp2+4)/24;
 days_past_currentyear = days % 365;
 years = days / 365;
 for(i=1970;i<=(1970+years);i++)
 {
 if ((i % 4) == 0)
 days_past_currentyear--;
 }
 if((1970+years % 4) != 0)
 {
 if(days_past_currentyear >=1 && days_past_currentyear <=31)
 {
 month=1; //JAN
 date = days_past_currentyear;
 }
 else if (days_past_currentyear >31 && days_past_currentyear <= 59)
 {
 month = 2;
 date = days_past_currentyear - 31; 
 }
 else if (days_past_currentyear >59 && days_past_currentyear <= 90)
 {
 month = 3;
 date = days_past_currentyear - 59;
 }
 else if (days_past_currentyear >90 && days_past_currentyear <= 120)
 {
 month = 4;
 date = days_past_currentyear - 90;
 }
 else if (days_past_currentyear >120 && days_past_currentyear <= 151)
 {
 month = 5;
 date = days_past_currentyear - 120;
 }
 else if (days_past_currentyear >151 && days_past_currentyear <= 181)
 {
 month = 6;
 date = days_past_currentyear - 151;
 }
 else if (days_past_currentyear >181 && days_past_currentyear <= 212)
 {
 month = 7;
 date = days_past_currentyear - 181;
 }
 else if (days_past_currentyear >212 && days_past_currentyear <= 243)
 {
 month = 8;
 date = days_past_currentyear - 212;
 }
 else if (days_past_currentyear >243 && days_past_currentyear <= 273)
 {
 month = 9;
 date = days_past_currentyear - 243;
 }
 else if (days_past_currentyear >273 && days_past_currentyear <= 304)
 {
 month = 10;
 date = days_past_currentyear - 273;
 }
 else if (days_past_currentyear >304 && days_past_currentyear <= 334)
 {
 month = 11;
 date = days_past_currentyear - 304;
 }
 else if (days_past_currentyear >334 && days_past_currentyear <= 365)
 {
 month = 12;
 date = days_past_currentyear - 334;
 }
 
 // printk(KERN_ALERT "month=%d date=%d year=%d",month,date,(1970+years));

 }
 // for leap years..
 else
 {
 if(days_past_currentyear >=1 && days_past_currentyear <=31)
 {
 month=1; //JAN
 date = days_past_currentyear;
 }
 else if (days_past_currentyear >31 && days_past_currentyear <= 60)
 {
 month = 2;
 date = days_past_currentyear - 31;
 }
 else if (days_past_currentyear >60 && days_past_currentyear <= 91)
 {
 month = 3;
 date = days_past_currentyear - 60;
 }
 else if (days_past_currentyear >91 && days_past_currentyear <= 121)
 {
 month = 4;
 date = days_past_currentyear - 91;
 }
 else if (days_past_currentyear >121 && days_past_currentyear <= 152)
 {
 month = 5;
 date = days_past_currentyear - 121;
 }
 else if (days_past_currentyear >152 && days_past_currentyear <= 182)
 {
 month = 6;
 date = days_past_currentyear - 152;
 }
 else if (days_past_currentyear >182 && days_past_currentyear <= 213)
 {
 month = 7;
 date = days_past_currentyear - 182;
 }
 else if (days_past_currentyear >213 && days_past_currentyear <= 244)
 {
 month = 8;
 date = days_past_currentyear - 213;
 }
 else if (days_past_currentyear >244 && days_past_currentyear <= 274)
 {
 month = 9;
 date = days_past_currentyear - 244;
 } 
 else if (days_past_currentyear >274 && days_past_currentyear <= 305)
 {
 month = 10;
 date = days_past_currentyear - 274;
 }
 else if (days_past_currentyear >305 && days_past_currentyear <= 335)
 {
 month = 11;
 date = days_past_currentyear - 305;
 }
 else if (days_past_currentyear >335 && days_past_currentyear <= 366)
 {
 month = 12;
 date = days_past_currentyear - 335;
 }
 // printk(KERN_ALERT "\nmonth=%d date=%d year=%d",month,date,(1970+years));
 }
 log_filename[0]=(month/10)+48; // Convert into Char from Int
 log_filename[1]=(month%10)+48;
 log_filename[3]=(date/10)+48;
 log_filename[4]=(date%10)+48;
 tmp1 = ((1970+years) % 10) + 48;
 log_filename[9]= tmp1;
 tmp1 = (1970+years)/ 10;
 tmp2 = tmp1 % 10;
 log_filename[8]= tmp2 + 48;
 tmp1 = tmp1 / 10;
 tmp2 = tmp1 % 10;
 log_filename[7]=tmp2 + 48;
 tmp1 = tmp1 / 10;
 log_filename[6]= tmp1+48;
 log_filename[10]='\0';

} 

short unsigned int my_ntoh(short unsigned int src_port)
{
	short unsigned int t,t1,t2;
	t = (src_port >> 8);
	t1 = (src_port << 8);
	t2 = t|t1;
	return(t2);
}

char *inet_ntoa(struct in_addr inn)
{
	static char m[18];
	register char *m1;
	m1 = (char *)&inn;
	#define UCC(m) (((int)m)&0xff)
	(void)snprintf(m, sizeof(m),"%u.%u.%u.%u", UCC(m1[0]), UCC(m1[1]), UCC(m1[2]), UCC(m1[3]));return(m);
}

module_init(my_init); 
module_exit(my_exit);              

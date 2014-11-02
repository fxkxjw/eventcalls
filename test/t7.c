#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>

int main(int argc, char **argv){
	if (argc != 5) { /* input arguments count wrong */
		printf("input error\n");
		return 0;
	}
	
	pid_t pid;
	int eid;
	int uid;
	int gid;
	int command;

	pid = getpid();

	/* Read the input arguements */
	uid = atoi(argv[1]);
	gid = atoi(argv[2]);
	eid = atoi(argv[3]);
	command = atoi(argv[4]);

	/* Change the effective uid and gid of the process */
	setegid(gid);
	seteuid(uid);
	printf("euid: %d, egid: %d\n", geteuid(),getegid());
	printf("The process id is %d\n", pid);
	
	/* Three commands */
	switch(command){

	/* doeventwait */
	case 183: 
		printf("Process %d is waiting on event %d\n", pid, eid);
		if(syscall(183, eid) == -1){
			printf("Fail in waiting\n");
			return 1;
		}
		break;

	/* doeventsig */
	case 184: 
		printf("Process %d calls doeventsig to wake up processes waiting on event %d\n", pid, eid);
		if(syscall(184, eid) == -1){
			printf("Fail in singal\n");
			return 1;
		}
		break;

	/* doeventclose */
	case 182: 
		printf("Process %d is closing the event %d\n", pid, eid);
		if(syscall(182, eid) == -1){
			printf("Fail in closing event %d\n", eid);
			return 1;
		}
		break;

	default: printf("Input command is wrong\n");
		break;
	}

	return 0;
}

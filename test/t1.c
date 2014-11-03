#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
/* creat event, then wait on it */
int main(){
	pid_t pid;
	int eid;
	/* creat event */
	eid = syscall(181);
	printf("the event ID is %d\n", eid);
	
	pid = getpid();
	printf("pid: %d\n", pid);
	/* doeventwait */
	syscall(183, eid);
	return 0;
}

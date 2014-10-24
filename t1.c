#include <linux/eventcalls.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

int main(){
	int eid;
	eid = syscall(181);	// sys_doeventopen() didn't contain error conditions
	printf("the event ID is %d\n", eid);
	return 0;
}

//aaaaaaaaaaaaa
// hello

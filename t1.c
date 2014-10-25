#include <linux/eventcalls.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

// find the state of a process with given pid
int main (int argc, char **argv)
{
        if (argc != 2) { /* input arguments count wrong */
                printf("input error\n");
                return 0;
        }

        pid_t pid;
        pid = atoi (argv[1]);

        struct prinfo *p =(struct prinfo*) malloc(sizeof(struct prinfo));
        p->pid = pid;

        if (syscall(299, p)!=0){
                printf("ERROR\n");
                exit(0);
        }
        printf("state: %d\n", p->state);
        return 0;
}

// creat a event and then wait for it
int main(){
        pid_t pid;
        int eid;
        eid = syscall(181);    
        printf("the event ID is %d\n", eid);
        pid = getpid();
        printf("pid: %d\n", pid);
        syscall(183, eid);
return 0;
}

// doeventwait
int main (int argc, char **argv)
{
        if (argc != 2) { /* input arguments count wrong */
                printf("input error\n");
                return 0;
        }
        int eid,sig;
        eid = atoi (argv[1]);
        sig = syscall(184, eid);
        return 0;
}

// doeventclose
int main(int argc, char **argv){
        if(argc != 2){
                printf("Input error\n");
                return 0;
        }
        int eid;
        eid = atoi (argv[1]);
        syscall(182, eid);

        return 0;
}

//aaaaaaaaaaaaa
// hello
// bbbbbbbbbbbbbbbb

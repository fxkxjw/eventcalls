#include <linux/eventcalls.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

//test the doeventstat
int main(){
        struct prinfo *p =(struct prinfo*) malloc(sizeof(struct prinfo));
        uid_t *UID = 0;
        gid_t *GID = 0;
        int *UIDFlag = 0;
        int *GIDFlag = 0;
        pid_t pid;
        int eid;
        pid = getpid();
        printf("The process id is %d\nUID is %d\nGID is %d\n",pid,getuid(),getgid());

        eid = syscall(181);     //Creat event
        if(syscall(214, eid, UID, GID, UIDFlag, GIDFlag) == -1){        //Read the info of the event with given eid
                printf("doeventstat fail\n");
                return 0;
        }
        printf("The first event ID is: %d\n UID: %d; UIDFlag: %d; GID: %d; GIDFlag: %d;",eid, *UID, *UIDFlag, *GID, *GIDFlag);


//Change UID to 100, GID to 101, UIDFlag to 102, GIDFlag to 103
        if(syscall(205, eid, 100, 101) == -1){  //Change the UID to 100, GID to 101
                printf("doeventchown fail\n");
                return 0;
        }
        if(syscall(214, eid, UID, GID, UIDFlag, GIDFlag) == -1){        //Read the info of the event with given eid
                printf("doeventstat fail\n");
                return 0;
        }
        printf("The first event ID is: %d\n UID: %d; UIDFlag: %d; GID: %d; GIDFlag: %d;",eid, *UID, *UIDFlag, *GID, *GIDFlag);

        if(syscall(211, eid, 102, 103) == -1){  //Change the UIDFlag to 102, GIDFlag to 103
                printf("doeventchmod fail\n");
                return 0;
        }
        if(syscall(214, eid, UID, GID, UIDFlag, GIDFlag) == -1){        //Read the info of the event with given eid
                printf("doeventstat fail\n");
                return 0;
        }
        printf("The first event ID is: %d\n UID: %d; UIDFlag: %d; GID: %d; GIDFlag: %d;",eid, *UID, *UIDFlag, *GID, *GIDFlag);



return 0;
}






// test the doeventinfo 
int main(int argc, char **argv){
        int num,num2,i;
        int a[10];
        num = syscall(185,0,NULL);
        printf("Numbers of events: %d\n", num);
        //eIDs = (int *)malloc(num * sizeof(int));
        num2 = syscall(185,num,a);
        printf("Second return number: %d\n", num2);
        for(i=0;i<num;i++){
                printf("Event ID : \n", a[i]);
        }
        return 0;
}




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


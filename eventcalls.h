//eventcalls.h

#ifndef __EVENT_H
#define __EVENT_H

// headers from timer.c
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/pid_namespace.h>
#include <linux/notifier.h>
#include <linux/thread_info.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/posix-timers.h>
#include <linux/cpu.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/tick.h>
#include <linux/kallsyms.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/div64.h>
#include <asm/timex.h>
#include <asm/io.h>

#include <linux/kernel.h>

#include <linux/list.h>
#include <linux/wait.h>
#include <linux/spinlock.h>



struct event
{
    int UID;
    int GID;
    char user_sig_enable;
    char group_sig_enable;

    int eventID;

    struct list_head eventID_list;  //implement a kernel double-linked list fo events

    wait_queue_head_t wait_queue;   //implement a wait queue of processes waiting on the event

    int go_aheads;
};


//return the length of the list with given list_head
int count_event(struct list_head * head);   

//return a pointer to the event with given event ID
//return NULL if the event with the given event ID is not found
struct event * get_event(int eventID);

//initialize a global event as the head of a linked list of other events
void doevent_init();

//create a new event
//create a new event id
//implement event is list as kernel linked list
//add the new event id to the linked list
//return event id on success
//return -1 on failure
asmlinkage long sys_doeventopen();

//destroy the event with the given event ID
//signal any process waiting on the event to leave the event
//return the number of processed signaled on success
//return -1 on failure
asmlinkage long sys_doeventclose(int eventID);

//block process until the event is signaled
//add the calling process to the wait queue for the event with the given event ID
//return 1 on success
//return -1 on failure
asmlinkage long sys_doeventwait(int eventID);

//unblocks all waiting processes
//ignoreed if no processes are blocked
//each event ID will have its own wait queue of processes
//wake up all processes in that queue
//return the number of processes signaled on success
//return -1 on failure
asmlinkage long sys_doeventsig(int eventID);

//fill in the array fo integers pointed to by eventIDs with the current set of active event IDs
//num is the number of integers which the memory pointed to by eventIDs can hold
//return the number of active events on success
//return -1 on failure
asmlinkage long sys_doeventinfo(int num, int * eventIDs);

extern rwlock_t eventID_list_lock;  //provide read write lock to evnetID list
extern struct event global_event;   //provide the main list
extern bool event_initialized;  //indicate if the global event has been initialized

#endif

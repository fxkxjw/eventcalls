// linux/include/linux/eventcalls.h

#ifndef __EVENT_H
#define __EVENT_H

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

#include <linux/sched.h>    // wake_up()
#include <linux/list.h> //list_head
#include <linux/wait.h> //wait_queue_head_t
#include <linux/spinlock.h>
#include <linux/types.h>    //uid_t gid_t


struct event
{
    uid_t UID;
    gid_t GID;
    int UIDFlag;
    int GIDFlag;

    int eventID;    //eventID should be positive integers

    struct list_head eventID_list;  //implement a kernel double-linked list fo events

    wait_queue_head_t wait_queue;   //implement a wait queue of processes waiting on the event

    /*
     * Wait condition for each task is that
     * wait_stage remains unchanged from the moment the value of wait_stage is fetched.
     * wait_stage is initialize to 0
     * and increases by 1 each time sys_doeventsig() is called.
     */
    int wait_stage;
};


/*
 * Return the length of the list with given list_head.
 * Remember to call read_lock before.
 */
int get_list_length(struct list_head * head);   

/*
 * Return a pointer to the event with given event ID.
 * Return NULL if the event with the given event ID is not found.
 * Remember to call read_lock before.
 */
struct event * get_event(int eventID);

/*
 * Initialize a global event as the head of a linked list of other events.
 * This function should be called in function start_kernel() in linux/init/main.c at kernel boot.
 */
void doevent_init();

/*
 * Create a new event and assign an event ID to it.
 * Add the new event to the global event list.
 * Return event id on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventopen();

/*
 * Wake up all tasks in the waiting queue of the event with given eventID.
 * Remove the event from global event list.
 * Free memory which hold the event.
 * Return the number of processed signaled on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventclose(int eventID);

/*
 * Make the calling tasks wait in the wait queue of the event with the given eventID.
 * Rreturn 0 on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventwait(int eventID);

/*
 * Wake up all tasks waiting in the event with the given event ID.
 * Remove all tasks from waiting queue.
 * Return the number of processes signaled on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventsig(int eventID);

/*
 * If eventIDs != NULL && num >= event_count, copy all event IDs to user array pointed to by eventIDs.
 * If eventIDs == NULL || num < event_count, do not copy.
 * Return the number of active events on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventinfo(int num, int * eventIDs);

/*
 * Change the UID and GID of the event with the given eventID to the given UID and GID.
 * Return 0 on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventchown(int eventID, uid_t UID, gid_t GID);

/*
 * Change the user signal enable bit to UIDFlag and the group signal enable bit to GIDFlag.
 * Return 0 on success.
 * Retutn -1 on failure.
 */
asmlinkage long sys_doeventchmod(int eventID, int UIDFlag, int GIDFlag);

/*
 * Place the UID, GID, User Signal Enable Bit and Group Signal Enable Bit into the memory pointed to by UID, GID, UIDFlag and GIDFlag respectively.
 * Return 0 on success.
 * Return -1 on filure.
 */
asmlinkage long sys_doeventstat(int eventID, uid_t * UID, gid_t * GID, int * UIDFlag, int * GIDFlag);


extern rwlock_t eventID_list_lock;  //provide read write lock to evnetID list
extern struct event global_event;   //provide the main list
extern bool event_initialized;  //indicate if the global event has been initialized

#endif

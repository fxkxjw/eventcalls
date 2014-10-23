// linux/kernel/eventcalls.c


#include <linux/eventcalls.h>

struct event global_event;
rwlock_t eventID_list_lock;
bool event_initialized;

//return the length of the list with given list_head
int count_event(struct list_head * head)
{
    int length = 0;
    struct list_head * pos;
    list_for_each(pos, head) length++;
    return length;
}


//return a pointer to the event with given event ID
//return NULL if the event with the given event ID is not found 
struct event * get_event(int eventID)
{
    //iterate global event list
    struct event * pos;
    list_for_each_entry(pos, &global_event.eventID_list, eventID_list) {
        if (pos->eventID == eventID) {
            return pos;
        }
    }
    return (struct event *) NULL;
}


//initialize a global event as the head of a linked list of other events
void doevent_init()
{
    eventID_list_lock = RW_LOCK_UNLOCKED;

    INIT_LIST_HEAD(&global_event.eventID_list);

    global_event.eventID = 0;

    init_waitqueue_head(&global_event.wait_queue);
    event_initialized = true;
}


//create a new event
//create a new event id
//implement event is list as kernel linked list
//add the new event id to the linked list
//return event id on success
//return -1 on failure
asmlinkage long sys_doeventopen()
{
    struct event * new_event = kmalloc(sizeof(struct event), GFP_KERNEL);

    // initialize event list
    INIT_LIST_HEAD(&(new_event->eventID_list));

    // add event to the main list
    unsigned long flags;

    write_lock_irqsave(&eventID_list_lock, flags);  //lock write
   
    list_add_tail(&(new_event->eventID_list), &global_event.eventID_list);
    int max_id = list_entry((new_event->eventID_list).prev, struct event, eventID_list)->eventID;
    new_event->eventID = max_id + 1;
    init_waitqueue_head(&(new_event->wait_queue));
    
    write_unlock_irqrestore(&eventID_list_lock, flags); //unlock write

    return new_event->eventID;
}


//destroy the event with the given event ID
//signal any process waiting on the event to leave the event
//return the number of processed signaled on success
//return -1 on failure
asmlinkage long sys_doeventclose(int eventID)
{
    if (eventID == 0) return -1;
    
    unsigned long flags;
    read_lock_irqsave(&eventID_list_lock, flags);   //lock read
    
    struct event * this_event = get_event(eventID);
    
    read_unlock_irqrestore(&eventID_list_lock, flags);  //unlock read

    if (this_event == NULL) return -1;

    int processes_signaled = sys_doeventsig(eventID);

    write_lock_irqsave(&eventID_list_lock, flags);  //lock write
    list_del(&(this_event->eventID_list));
    write_unlock_irqrestore(&eventID_list_lock, flags); //unlock write

    kfree(this_event);


    return processes_signaled;
}


//block process until the event is signaled
//add the calling process to the wait queue for the event with the given event ID
//return 1 on success
//return -1 on failure
asmlinkage long sys_doeventwait(int eventID)
{
    if (eventID == 0) return -1;

    unsigned long flags;  
    read_lock_irqsave(&eventID_list_lock, flags);   //lock read
    
    struct event * this_event = get_event(eventID);
    int x = this_event->go_aheads;
    
    read_unlock_irqrestore(&eventID_list_lock, flags);  //unlock read

    while (x == this_event->go_aheads) {
        interruptible_sleep_on(&(this_event->wait_queue));
    }

    return 0;
}

//unblocks all waiting processes
//ignoreed if no processes are blocked
//each event ID will have its own wait queue of processes
//wake up all processes in that queue
//return the number of processes signaled on success
//return -1 on failure
asmlinkage long sys_doeventsig(int eventID)
{
    if (eventID == 0) return -1;

    unsigned long flags;
    
    write_lock_irqsave(&eventID_list_lock, flags);  //lock write
    
    struct event * this_event = get_event(eventID);
    
    if (this_event == NULL) return -1;
    
    this_event->go_aheads++;
    
    int processes_signaled = count_event(&(this_event->wait_queue.task_list));   //get the number of processes waiting on this event
    
    wake_up_interruptible(&(this_event->wait_queue));
    
    write_unlock_irqrestore(&eventID_list_lock, flags); // unlock write

    return processes_signaled;
}



//if eventIDs != NULL && num >= event_count, copy all event IDs to user array pointed to by eventIDs
//if eventIDs == NULL || num < event_count, do not copy
//return the number of active events on success
//return -1 on failure
asmlinkage long sys_doeventinfo(int num, int * eventIDs)
{

    int event_count = count_event(&global_event.eventID_list);
    //arguments exception check
    if (num < event_count || eventIDs == NULL)
        return event_count;
    

    //kmalloc an array for storing event IDs
    int * sys_eventIDs;
    if ((sys_eventIDs = kmalloc(event_count * sizeof(int), GFP_KERNEL)) == NULL) {
        printk("error kmalloc\n");
        return -1;
    }

   
    //insert all event IDs to array pointed to by sys_eventIDs
    int i = 0;
    struct event * pos; 
    list_for_each_entry(pos, &global_event.eventID_list, eventID_list) {    
            
        //do not insert or count global_event whose eventID is 0
        if ((pos->eventID) != 0) {  
            *(sys_eventIDs + i) = pos->eventID;   
        }
    }

    //copy to user
    if (copy_to_user(eventIDs, sys_eventIDs, event_count * sizeof(int)) != 0) {
        printk("error copy_to_user\n");
        return -1;
    }
    
    return event_count;
}


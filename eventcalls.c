// linux/kernel/eventcalls.c


#include <linux/eventcalls.h>



struct event global_event;
rwlock_t eventID_list_lock;
bool event_initialized;






/*
 * Return the length of the list with given list_head.
 * Remember to call read_lock before.
 */
int get_list_length(struct list_head * head)
{
    int length = 0;
    struct list_head * pos;
    list_for_each(pos, head) length++;
    return length;
}







/*
 * Return a pointer to the event with given event ID.
 * Return NULL if the event with the given event ID is not found.
 * Remember to call read_lock before.
 */
struct event * get_event(int eventID)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }
    
    struct event * pos;
    list_for_each_entry(pos, &global_event.eventID_list, eventID_list) {
        if (pos->eventID == eventID) {
            return pos;
        }
    }
    return (struct event *) NULL;
}








/*
 * Initialize a global event as the head of a linked list of other events.
 * Set event_initialized true.
 * This function should be called in function start_kernel() in linux/init/main.c at kernel boot.
 */
void doevent_init()
{
    eventID_list_lock = RW_LOCK_UNLOCKED;

    INIT_LIST_HEAD(&global_event.eventID_list);

    global_event.eventID = 0;

    init_waitqueue_head(&global_event.wait_queue);
    event_initialized = true;
}







/*
 * Create a new event and assign an event ID to it.
 * Add the new event to the global event list.
 * Return event id on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventopen()
{

    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }

    struct event * new_event = kmalloc(sizeof(struct event), GFP_KERNEL);

    INIT_LIST_HEAD(&(new_event->eventID_list)); // Initialize event list entry.
    new_event->wait_stage = 0;  // Initialize wait_stage.

    unsigned long flags;


    write_lock_irqsave(&eventID_list_lock, flags);  // Lock write.
    list_add_tail(&(new_event->eventID_list), &global_event.eventID_list);  // Add new_event to the tail of event list
    int max_id = list_entry((new_event->eventID_list).prev, struct event, eventID_list)->eventID; // Find preceding event's ID.
    new_event->eventID = max_id + 1;    // Assign eventID to new_event. No duplicate!
    init_waitqueue_head(&(new_event->wait_queue));  // Initialize wait queue.
    write_unlock_irqrestore(&eventID_list_lock, flags); // Unlock write.


    return new_event->eventID;
}






/*
 * Wake up all tasks in the waiting queue of the event with given eventID.
 * Remove the event from global event list.
 * Free memory which hold the event.
 * Return the number of processed signaled on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventclose(int eventID)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }


    if (eventID <= 0) return -1;    //check argument exception
    
    unsigned long flags;



    read_lock_irqsave(&eventID_list_lock, flags);   //lock read
    struct event * this_event = get_event(eventID); //search for event in event list
    read_unlock_irqrestore(&eventID_list_lock, flags);  //unlock read

    
    // If event not found.
    if (this_event == NULL) {
        printk("error: event not found. eventID = %d\n", eventID);
        return -1;
    }


    /*
     * Wake up all tasks in the waiting queue of the event.
     * Remove the tasks from the waiting queue.
     */
    int processes_signaled = sys_doeventsig(eventID);

   
    
    write_lock_irqsave(&eventID_list_lock, flags);  //lock write
    list_del(&(this_event->eventID_list));  //delete event from event list
    write_unlock_irqrestore(&eventID_list_lock, flags); //unlock write



    kfree(this_event);  //remember to free memory

    return processes_signaled;
}






/*
 * Make the calling tasks wait in the wait queue of the event with the given eventID.
 * Rreturn 0 on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventwait(int eventID)
{

    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }


    if (eventID <= 0) return -1;

    unsigned long flags;  
    
    

    read_lock_irqsave(&eventID_list_lock, flags);   // Lock read
    struct event * this_event = get_event(eventID); // Search for the event in event list
    read_unlock_irqrestore(&eventID_list_lock, flags);  // Unlock read
    
    // If event not found.
    if (this_event == NULL) {
        printk("error: event not found. eventID = %d\n", eventID);
        return -1;
    }

    /*
     * If wait_stage changes between execution of this line and execution of wait_event()
     * the task will not wait on the event.
     */
    int x = this_event->wait_stage;

    /*
     * Wait in queue until wait_stage changes.
     * wait_stage is check each time the wait queue is waked up.
     * This function includes adding task to wait queue and removing from queue.
     * Remember to call wake_up() each time after wait_stage changes to check the value of wait_stage.
     */
    wait_event(this_event->wait_queue, this_event->wait_stage != x);


    return 0;
}






/*
 * Wake up all tasks waiting in the event with the given event ID.
 * Remove all tasks from waiting queue.
 * Return the number of processes signaled on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventsig(int eventID)
{

    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }


    if (eventID <= 0) return -1;    // Check argument exceptions.

    unsigned long flags;
    


    read_lock_irqsave(&eventID_list_lock, flags);  // Lock read. 
    struct event * this_event = get_event(eventID); // Search for event in event list.
    if (this_event == NULL) return -1;  // Event not found.
    
    this_event->wait_stage++; // Wake up tasks waiting on this stage.    
    int processes_signaled = get_list_length(&(this_event->wait_queue.task_list));   // Get the number of processes waiting on this event.
    read_unlock_irqrestore(&eventID_list_lock, flags); // Unlock read.
    

    wake_up(&(this_event->wait_queue)); // For all waiting tasks to check if wait_stage has been changed since they start sleep, if yes, wake up!
    

    return processes_signaled;
}




/*
 * If eventIDs != NULL && num >= event_count, copy all event IDs to user array pointed to by eventIDs.
 * If eventIDs == NULL || num < event_count, do not copy.
 * Return the number of active events on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventinfo(int num, int * eventIDs)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }


    int event_count = get_list_length(&global_event.eventID_list);
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





/*
 * Change the UID and GID of the event with the given eventID to the given UID and GID.
 * Return 0 on success.
 * Return -1 on failure.
 */
asmlinkage long sys_doeventchown(int eventID, uid_t UID, gid_t GID)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }


    return 0;
}





/*
 * Change the user signal enable bit to UIDFlag and the group signal enable bit to GIDFlag.
 * Return 0 on success.
 * Retutn -1 on failure.
 */
asmlinkage long sys_doeventchmod(int eventID, uid_t * UID, gid_t GID, int * UIDFlag, int * GIDFlag)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }


    return 0;
}





/*
 * Place the UID, GID, User Signal Enable Bit and Group Signal Enable Bit into the memory pointed to by UID, GID, UIDFlag and GIDFlag respectively.
 * Return 0 on success.
 * Return -1 on filure.
 */
asmlinkage long sys_doeventstat(int eventID, uid_t * UID, gid_t GID, int * UIDFlag, int * GIDFlag)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error: event not initialized\n");
        return -1;
    }


    return 0;
}


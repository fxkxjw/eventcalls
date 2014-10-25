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
    // Check arguemnts.
    if (head == NULL) {
        printk("error get_list_length(): invalid argument\n");
        return -1;
    }

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
        printk("error get_event(): event not initialized\n");
        return -1;
    }

    // Check arguments.
    if (eventID == NULL || eventID <= 0) {
        printk("error get_event(): invalid eventID\n");
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
        printk("error sys_doeventopen(): event not initialized\n");
        return -1;
    }

    struct event * new_event = kmalloc(sizeof(struct event), GFP_KERNEL);

    // Initialize attributes of new_event.
    new_event->UID = current->real_cred->uid;  
    new_event->GID = current->real_cred->gid;
    new_event->UIDFlag = 1;
    new_event->GIDFlag = 1;
    INIT_LIST_HEAD(&(new_event->eventID_list)); // Initialize event list entry.
    new_event->wait_stage = 0;

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
 * Access denied:
 *  uid != 0 && (uid != event->UID || event->UIDFlag == 0) && (gid != event->GID || event->GIDFlag == 0)
 */
asmlinkage long sys_doeventclose(int eventID)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error sys_doeventclose(): event not initialized\n");
        return -1;
    }

    // Check arguments.
    if (eventID == NULL || eventID <= 0) {
        printk("error sys_doeventclose(): invalid eventID\n");
        return -1;
    }
    


    unsigned long flags;
    read_lock_irqsave(&eventID_list_lock, flags);   //lock read
    struct event * this_event = get_event(eventID); //search for event in event list
    read_unlock_irqrestore(&eventID_list_lock, flags);  //unlock read

    
    // If event not found.
    if (this_event == NULL) {
        printk("error sys_doeventclose(): event not found. eventID = %d\n", eventID);
        return -1;
    }

    // Check accessibility.
    uid_t uid = current->real_cred->uid;
    gid_t gid = current->real_cred->gid;
    if (uid != 0 && (uid != this_event->UID || this_event->UIDFlag == 0) && (gid != this_event->GID || this_event->GIDFlag == 0)) {
        printk("sys_doeventclose(): access denied\n");
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
 * Access denied:
 *  uid != 0 && (uid != event->UID || event->UIDFlag == 0) && (gid != event->GID || event->GIDFlag == 0)
 */
asmlinkage long sys_doeventwait(int eventID)
{

    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error sys_doeventwait(): event not initialized\n");
        return -1;
    }

    // Check arguments.
    if (eventID == NULL || eventID <= 0) {
        printk("error sys_doeventwait(): invalid eventID\n");
        return -1;
    }



    unsigned long flags;  
    read_lock_irqsave(&eventID_list_lock, flags);   // Lock read
    struct event * this_event = get_event(eventID); // Search for the event in event list
    read_unlock_irqrestore(&eventID_list_lock, flags);  // Unlock read
    

    // If event not found.
    if (this_event == NULL) {
        printk("error sys_doeventwait(): event not found. eventID = %d\n", eventID);
        return -1;
    }


    // Check accessibility.
    uid_t uid = current->real_cred->uid;
    gid_t gid = current->real_cred->gid;
    if (uid != 0 && (uid != this_event->UID || this_event->UIDFlag == 0) && (gid != this_event->GID || this_event->GIDFlag == 0)) {
        printk("sys_doeventwait(): access denied\n");
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
     * Also remember to release any lock before calling wait_event(), or there could be DEADLOCK!
     */
    wait_event(this_event->wait_queue, this_event->wait_stage != x);


    return 0;
}






/*
 * Wake up all tasks waiting in the event with the given event ID.
 * Remove all tasks from waiting queue.
 * Return the number of processes signaled on success.
 * Return -1 on failure.
 * Access denied:
 *  uid != 0 && (uid != event->UID || event->UIDFlag == 0) && (gid != event->GID || event->GIDFlag == 0)
 */
asmlinkage long sys_doeventsig(int eventID)
{

    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error sys_doeventsig(): event not initialized\n");
        return -1;
    }

    // Check arguments.
    if (eventID == NULL || eventID <= 0) {
        printk("error sys_doeventsig(): invalid eventID\n");
        return -1;
    } 


    unsigned long flags;
    read_lock_irqsave(&eventID_list_lock, flags);  // Lock read. 
    struct event * this_event = get_event(eventID); // Search for event in event list.
    read_unlock_irqrestore(&eventID_list_lock, flags); // Unlock read.
    


    // If event not found.
    if (this_event == NULL) {
        printk("error sys_doeventsig(): event not found. eventID = %d\n", eventID);
        return -1;
    }



    // Check accessibility.
    uid_t uid = current->real_cred->uid;
    gid_t gid = current->real_cred->gid;
    if (uid != 0 && (uid != this_event->UID || this_event->UIDFlag == 0) && (gid != this_event->GID || this_event->GIDFlag == 0)) {
        printk("sys_doeventsig(): access denied\n");
        return -1;
    }



    
    this_event->wait_stage++; // Wake up tasks waiting on this stage.    
    int processes_signaled = get_list_length(&(this_event->wait_queue.task_list));   // Get the number of processes waiting on this event.
    

    wake_up(&(this_event->wait_queue)); // For all waiting tasks to check if wait_stage has been changed since they start sleep.
    

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
        printk("error sys_doeventinfo(): event not initialized\n");
        return -1;
    }

    


    unsigned long flags;    
    read_lock_irqsave(&eventID_list_lock, flags);  // Lock read.
    int event_count = get_list_length(&global_event.eventID_list);  // Get event count.

    //kmalloc an array for storing event IDs
    int * sys_eventIDs;
    if ((sys_eventIDs = kmalloc(event_count * sizeof(int), GFP_KERNEL)) == NULL) {
        printk("error sys_doeventinfo(): kmalloc()\n");
        return -1;
    }

    //insert all event IDs to array pointed to by sys_eventIDs
    int i = 0;
    struct event * pos; 
     
    list_for_each_entry(pos, &global_event.eventID_list, eventID_list) {    
            
        //do not insert or count global_event whose eventID is 0
        if ((pos->eventID) != 0) {  
            *(sys_eventIDs + i++) = pos->eventID;   
        }
    }

    read_unlock_irqrestore(&eventID_list_lock, flags); // Unlock read.

    
    
        
    // Check arguments.
    if (num < event_count || eventIDs == NULL) {
        kfree(sys_eventIDs);
        return event_count;
    }
    
    

    // Copy to user.
    if (copy_to_user(eventIDs, sys_eventIDs, event_count * sizeof(int)) != 0) {
        printk("error sys_doeventinfo(): copy_to_user()\n");
        return -1;
    }

    kfree(sys_eventIDs);
    
    return event_count;
}





/*
 * Change the UID and GID of the event with the given eventID to the given UID and GID.
 * Return 0 on success.
 * Return -1 on failure.
 * Access denied:
 *  uid != event->UID
 */
asmlinkage long sys_doeventchown(int eventID, uid_t UID, gid_t GID)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error sys_doeventchown(): event not initialized\n");
        return -1;
    }

    // Check arguments.
    if (eventID == NULL || eventID <= 0 || UID == NULL || GID == NULL) {
        printk("error sys_doeventchown(): invalid arguments\n");
        return -1;
    }

    unsigned long flags;    
    read_lock_irqsave(&eventID_list_lock, flags);   // Lock read
    struct event * this_event = get_event(eventID); // Search for the event in event list
    read_unlock_irqrestore(&eventID_list_lock, flags);  // Unlock read
    
    // If event not found.
    if (this_event == NULL) {
        printk("error sys_doeventchown(): event not found. eventID = %d\n", eventID);
        return -1;
    }


    // Check accessibility.
    uid_t uid = current->real_cred->uid;
    if (uid != this_event->UID) {
        printk("sys_doeventchown(): access denied\n");
        return -1;
    }



    this_event->UID = UID;
    this_event->GID = GID;

    return 0;
}





/*
 * Change event->UIDFlag to UIDFlag and event->GIDFlag to GIDFlag.
 * Return 0 on success.
 * Retutn -1 on failure.
 * Access denied:
 *  uid != event->UID
 */
asmlinkage long sys_doeventchmod(int eventID, int UIDFlag, int GIDFlag)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error sys_doeventchmod(): event not initialized\n");
        return -1;
    }

    // Check arguments.
    if (eventID == NULL || eventID <= 0 || UIDFlag == NULL || GIDFlag == NULL) {
        printk("error sys_doeventchmod(): invalid arguments\n");
        return -1;
    }

    unsigned long flags;    
    read_lock_irqsave(&eventID_list_lock, flags);   // Lock read
    struct event * this_event = get_event(eventID); // Search for the event in event list
    read_unlock_irqrestore(&eventID_list_lock, flags);  // Unlock read
    
    // If event not found.
    if (this_event == NULL) {
        printk("error sys_doeventchmod(): event not found. eventID = %d\n", eventID);
        return -1;
    }

    // Check accessibility.
    uid_t uid = current->real_cred->uid;
    if (uid != this_event->UID) {
        printk("sys_doeventchmod(): access denied\n");
        return -1;
    }

    this_event->UIDFlag = UIDFlag;
    this_event->GIDFlag = GIDFlag;

    return 0;
}





/*
 * Place the UID, GID, User Signal Enable Bit and Group Signal Enable Bit into the memory pointed to by UID, GID, UIDFlag and GIDFlag respectively.
 * Return 0 on success.
 * Return -1 on filure.
 */
asmlinkage long sys_doeventstat(int eventID, uid_t * UID, gid_t * GID, int * UIDFlag, int * GIDFlag)
{
    // Remember to check if event is initialized at kernel boot before actually doing anything
    if (event_initialized == false) {
        printk("error sys_doeventstat(): event not initialized\n");
        return -1;
    }

    // Check arguments.
    if (eventID == NULL || eventID <= 0) {
        printk("error sys_doeventstat(): invalid eventID\n");
        return -1;
    }

    unsigned long flags;    
    read_lock_irqsave(&eventID_list_lock, flags);   // Lock read
    struct event * this_event = get_event(eventID); // Search for the event in event list
    read_unlock_irqrestore(&eventID_list_lock, flags);  // Unlock read
    
    // If event not found.
    if (this_event == NULL) {
        printk("error sys_doeventstat(): event not found. eventID = %d\n", eventID);
        return -1;
    }


    if (copy_to_user(UID, this_event->UID, sizeof(uid_t)) != 0) {
        printk("error sys_doeventstat(): copy_to_user()\n");
        return -1;
    }

    if (copy_to_user(GID, this_event->GID, sizeof(gid_t)) != 0) {
        printk("error sys_doeventstat(): copy_to_user()\n");
        return -1;
    }

    if (copy_to_user(UIDFlag, this_event->UIDFlag, sizeof(int)) != 0) {
        printk("error sys_doeventstat(): copy_to_user()\n");
        return -1;
    }

    if (copy_to_user(GIDFlag, this_event->GIDFlag, sizeof(int)) != 0) {
        printk("error sys_doeventstat(): copy_to_user()\n");
        return -1;
    }


    return 0;
}


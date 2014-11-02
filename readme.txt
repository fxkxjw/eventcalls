	Machine problem 3: Test
	Test programs from t1.c to t5.c are created for basic test of the doevent system call that is the first version without access control. Test programs from t6.c to t9.c are added later to accomplish the entire function test. The state.c program uses the system call added in MP2 to find the state of the task with given pid. 
 	
	state.c : Find the stadus of the process with given pid.

	t1.c : Using system call 181 (doeventopen) to create event, print its eventID, then wait on it.

	t2.c : Print process id; using system call 183 (doeventwait) to wait for an event with given eventID.
		input example: ./t2 eventID

	t3.c : Using system call 184 (doeventsig) to wake up all the processes waiting for the event with given eventID.
		input example: ./t3 eventID

	t4.c : Using system call 182 (doeventclose) to wake up all the waiting processes, then close the event with given eventID.
		input example: ./t4 eventID

	t5.c : Using sys_doeventinfo 185 twice: first get the return number of events, then create a pointer with allocated memory; second get the return events id.	

	t6.c : create event with given UID, GID, UIDFlag, GIDFlag. 
		Input example: ./t6 UID GID UIDFlag GIDFlag

	t7.c : create process with given uid, gid, target eventID and the command type. 
		Input example: ./t7 uid gid eventID syscall_number
		syscall_number: 183: wait
			184: signal
			182: close

	t8.c : create process with given uid, target eventID and the UID & GID of the event to be changed to.
		Input example: ./t8 uid eventID UID GID
	
	t9.c : create process with given uid, target eventID and the UIDFlag & GIDFlag of the event to be changed to.
		Input example: ./t9 uid eid UIDFlag GIDFlag

	system call 181: doeventopen.
	system call 182: doeventclose.
	system call 183: doeventwait.
	system call 184: doeventsig.
	system call 205: doeventchown.
	system call 211: doeventchmod.
	system call 214: doeventstat.
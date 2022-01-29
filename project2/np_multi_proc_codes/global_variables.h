#ifndef global_variables_np_multi_proc_h
#define global_variables_np_multi_proc_h
#define MAX_CLIENTS 31
#define MAX_N_NUM_PIPE 1001
#define MAX_N_ORD_PIPE 5000
#define MAX_N_FORK 100
#define MAX_MESSAGE_LENGTH 1025
#define MAX_NICKNAME_LENGTH 21
#define MAX_IP_PORT_LENGTH 50
#define MAX_BUFFER 15001
class client_unit;
// each slave process will have its "own" global variable i.e. slave process 1's global variable != slave process 2's global variable. the key point is that one of these global variables will point to the shared memory so that different slave process can communicate with each other
// different slave process record different user_pipes information. we don't seem user_pipes as shared variable since process1's fd=3 != process2's fd=3. instead, we close user pipe read/write end immediately once finish reading/writing on it.

extern int client_alist_id;

// each slave process will have its own client variable. here set client as global variable to let each slabe process' signal handling function can identify this variable
extern client_unit *client;

// client_alist becomes from static variable in single-process to shared memory
extern client_unit *client_alist;

#endif /* global_variables_np_multi_proc_h */

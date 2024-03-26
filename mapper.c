/*
program runs as follows
    user enters:
        ./mapper commandFile bufferSize
    command file has n directoryPaths
    buffer size has size of buffer between map and red

*/
#define _DEFAULT_SOURCE 

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>

#define MAX_LINE_SIZE 1024
#define MAX_WORD_SIZE 256

// GLOBAL VARS
sem_t empty; // need to do sem_init(&empty, 0, buffsize)
sem_t full; // need to do sem_init(&full, 0, 0)
sem_t mutex; // need to do sem_init(&mutex, 0, 1)

int buf_size;



// STRUCT DEFINITIONS

// struct sent to reciver.c
// THIS IS WRONG
// message needs to be 

// maybe just make message

typedef struct message
{
    long type;
    char content[MAX_WORD_SIZE];
} message;

typedef struct node
{
    message to_send;
    struct node* next;
    struct node* prev;
} node;

typedef struct list
{
    node* head;
    node* tail;
    unsigned int size;
    unsigned int max_size;
} list;

typedef struct work_param_struct
{
    char f_n[MAX_WORD_SIZE];
    list* l;
} work_param_struct;

typedef struct send_param_struct
{
    int msg_q_id;
    key_t k;
    list* l;
}send_param_struct;

/*
typedef struct word_struct
{// message sent from mapper to reducer
    char word[MAX_WORD_SIZE];
    int count;
} word_struct;

typedef struct message
{
    long mtype;
    word_struct w;

} message;

// linked list node sending messages
typedef struct m_node 
{// node wrapper for message
    message data;
    struct m_node* next;
} m_node;

// node used as to_send buffer
typedef struct m_list
{
    m_node* head;
    m_node* tail;
    int size;
} m_list;

*/

// FUNCTION DECLARATIONS

// thread function for sending thread
void *sender(void *send_param);
// thread function for worker thread
void *worker(void *work_param);

// linked list functions
node* create_node(char word[MAX_WORD_SIZE]);
list* create_list(unsigned int max_size);
void list_destroy(list * l);
void list_add_tail(list* l, node* m);
node* list_rem_head(list* l);


// MAIN
int main(int argc, char **argv)
{
// CHECK IF ARGS ARE VALID
    if (argc != 3) 
    {
        printf("ERROR: wrong arguments\n\n");
        return -1;
    } // if file is run without commandFile bufferSize

// VARIABLE DECLARATION AND INITILIZATIONS
    int buff_size = atoi(argv[2]); // this is the max size of the buffer m_list
    printf("buff_size: %d\n\n\n\n", buff_size);
    sem_init(&empty, 0, buff_size);
    sem_init(&full, 0, 0);
    sem_init(&mutex, 0, 1);
    FILE *cmd_file = fopen(argv[1], "r");
    char dir_name[MAX_LINE_SIZE];
    pid_t x, wpid;
    int status = 0;
    message end_m;

    int message_queue_id;
    key_t key;
    if((key = ftok("mapper.c", 1)) == -1)
    {
        perror("ftok");
        exit(1);
    }

    if((message_queue_id = msgget(key, 0644 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(1);
    }


    printf("Parent process ID should be: %d\n\n", getpid());
    
    while(fscanf(cmd_file, "map %s\n\n", dir_name) != EOF)
    {
        // for each map command, fork
        x = fork();
        if(x == 0) 
        {// if recently forked childe process, close cmd file and move on
            fclose(cmd_file);
            break;
        }
        //continue
    }


    if(x > 0)
    {
        fclose(cmd_file);
        printf("I am the parent with pid = %d\n\n", getpid());
        // don't return, find proc wait function, do that then send message
        
        // make end message
        
        strcpy(end_m.content, "");// empty word could be a sign
        end_m.type = 1; // idk how to do this

        // wait for other processes to conclude
        while ((wpid = wait(&status)) > 0){printf("waiting...\n\n");};
        // send end_m in message queue

        printf("status: %d\n\n", wait(&status));

        if(msgsnd(message_queue_id, &end_m , MAX_WORD_SIZE, 0) == -1) 
        {
            perror("Error in msgsnd");
        }

        printf("parent process closing\n\n");
        exit(0);
    }
    else if(x == 0)
    {
        printf("I am child with pid = %d\n\n", getpid());
        printf("\tline: %s\n\n", dir_name); // line = directory path
        
        // make bounded buffer here
        list* buf = create_list(buff_size);

        // create sender thread
        pthread_t sender_tid;
        pthread_attr_t sender_attr;

        // do sender thread stuff
          /* get the default attributes */
        //pthread_attr_init(&sender_attr);

        /* create the thread */

        //pthread_create(&sender_tid, &sender_attr, sender, buf); //what does sender need?
        

        // for each file in command dir create worker thread
        pthread_t *worker_tid; // how big to makes these?
        pthread_attr_t *worker_attr;
        int num_workers = 0;
        int i = 0;

        // change names to show understanding-------------------------------------------------------------------------
        struct dirent *dir_ent;
        DIR *dir = opendir(dir_name);

        // idk why i needed to count num_workers
        // get num files
        while( (dir_ent = readdir(dir)) != NULL)
        {
            if(dir_ent->d_type == DT_REG) 
            {
                num_workers++;
            }
        }
        closedir(dir);

        /*
        char *files[num_workers];
        for(i = 0; i < num_workers; i++)
        {
            files[i] = malloc(sizeof(char) * MAX_LINE_SIZE);
        }
        */

        
        // not sure if i need to close and reopen
        // need to find num_workers to determine how big worker_(tid, attr) needed to be
        char* file_path;
        
        dir = opendir(dir_name);
        worker_tid = (pthread_t*)malloc(sizeof(pthread_t) * num_workers);
        worker_attr = (pthread_attr_t*)malloc(sizeof(pthread_attr_t) * num_workers);

        work_param_struct* wp;
        
        
        while((  (dir_ent = readdir(dir)) != NULL && i < num_workers)) // how to propperly assign readdir?
        {
            if(dir_ent->d_type == DT_REG)
            {
                // create file path
                file_path = malloc(sizeof(char) * MAX_LINE_SIZE);
                strcpy(file_path, dir_name);
                strcat(file_path, "/");
                strcat(file_path, dir_ent->d_name);

                wp = malloc(sizeof(work_param_struct));

                strcpy(wp->f_n, file_path);
                wp->l = buf;

                printf("filepath before thread creation: %s\n\n", file_path);
                //create thread
                
                pthread_create(&worker_tid[i], &worker_attr[i], worker, wp);// work param needs "line + filename"
                i++;
            }

        }

        send_param_struct* sp = malloc(sizeof(send_param_struct));
        sp->l = buf;
        sp->k = key;
        sp->msg_q_id = message_queue_id;

        pthread_attr_init(&sender_attr);
        pthread_create(&sender_tid, &sender_attr, sender, sp);



// WRAP UP VARIABLES AND POINTERS
// this doesn't happen until all of the threads are complete
        pthread_join(sender_tid, NULL);
        for(int i = 0; i < num_workers; i++)
        {
            pthread_join(worker_tid[i], NULL);
        }
        list_destroy(buf);
        sem_destroy(&empty);// workers wait on this to put struct in buffer
        sem_destroy(&full);// senders wait on this to send struct to reducer.c
        sem_destroy(&mutex);//
        closedir(dir);
        free(worker_tid);
        free(worker_attr);
        printf("child process: %d complete\n\n", getpid());
        exit(0);
    }

    //return 0;
}

// STRUCT DEFINITIONS

/*
struct send_p
{
    // idk might not need this
};

struct work_p
{
    char* file_path;
};
*/

// FUNCTION DEFINITIONS
// create buffer to share with reducer.c

void *sender(void *send_param_voidptr)
{// this needs to loop dummy
// how does sender know when workers are done?
// message queue needs to be made in the parent process, maybe even
    printf("sender thread %d starting\n\n", pthread_self());

    send_param_struct* sp = (send_param_struct *) send_param_voidptr;
    
    //list* buf = (list *) buff_void; 

    sem_wait(&full);
    sem_wait(&mutex);
    message m = list_rem_head(sp->l)->to_send;
    sem_post(&mutex);
    sem_post(&empty);

/*
    int message_queue_id;
    key_t key;
    
    if((key = ftok("mapper.c", 1)) == -1)
    {
        perror("ftok");
        exit(1);
    }

    if((message_queue_id = msgget(key, 0644 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(1);
    }
*/

    printf("key: %d\n\n", sp->k);

    if(msgsnd(sp->msg_q_id, &m , MAX_WORD_SIZE, 0) == -1) 
    {
        perror("Error in msgsnd");
    }


    printf("sender thread ending\n\n");
    pthread_exit(0);
}

// each worker gets a file to read and return data from
// I need work_param struct 


void *worker(void *work_param_voidptr)
{
    
    //work_param_struct wp = (work_param_struct) *work_param_voidptr;
    printf("worker thread %d starting\n\n", pthread_self());



    work_param_struct* wp = (work_param_struct *) work_param_voidptr;



    printf("thread %d file_name: %s\n\n", pthread_self(), wp->f_n);
    FILE* to_work = fopen(wp->f_n, "r");

    

    free(wp->f_n);

    char word_1[MAX_WORD_SIZE];
    while(fscanf(to_work, "%s", word_1) != EOF)
    {// this part is easy, so it's really hard
    // go through the file and count how many times each word happens
        //strcmp();
        
        node* new = create_node(word_1);
        //strcpy(new->data->message, word_1); // could also put this in fscanf
        
        // buffer and argv need to be passed, need to make work_param struct

        
        sem_wait(&empty);
        sem_wait(&mutex);
        list_add_tail(wp->l, new); // this needs to be passed?
        sem_post(&mutex);
        sem_post(&full);
    }

    printf("worker thread ending\n\n");
    pthread_exit(0);
}

node* create_node(char word[MAX_WORD_SIZE])
{
    node* m = malloc(sizeof(node));

    strcpy(m->to_send.content, word);
    m->to_send.type = 1;
    m->next = NULL;
    m->prev = NULL;

    return m;
}

list* create_list(unsigned int max_size)
{
    list* l = malloc(sizeof(list));
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
    l->max_size = max_size;
    return l;
}

void list_destroy(list* l)
{
    node *p = l->head;
    node *tmp;

    while(p != NULL)
    {
        tmp = p;
        p = p->next;
        free(tmp);
    }
    free(l);
}

void list_add_tail(list* l, node* m)
{
    if(l->size == l->max_size) 
    {
        printf("ERROR: adding to full list\n\n"); 
        return;
    }

    
    if(l->size == 0)
    {
        l->head = m;
        l->tail = m;
    }
    else
    {
        l->tail->next = m;
        m->prev = l->tail;
        l->tail = m;
    }
    l->size++;

}

node* list_rem_head(list* l)
{
    if(l->size == 0)
    { 
        printf("ERROR: removing from empty list\n\n");
        return NULL;
    }
    node* m = l->head;

    if(l->size == 1)
    {
        l->head = NULL;
        l->tail = NULL;
    }
    else//if(l->size != 1)
    {
        l->head = l->head->next;
        l->head->prev = NULL;

        m->next = NULL;
        m->prev = NULL;
    }
    
    l->size--;

    return m;
}

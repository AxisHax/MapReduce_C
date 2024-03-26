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
sem_t empty;
sem_t full;
sem_t mutex;

int buf_size;



// STRUCT DEFINITIONS
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
        printf("commence waiting\n\n");
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
        
    // variables needed for each process, shared between threads
        list* buf = create_list(buff_size);
        pthread_t sender_tid;
        pthread_attr_t sender_attr;
        pthread_t *worker_tid;
        pthread_attr_t *worker_attr;
        int num_workers = 0;
        int i = 0;
        struct dirent *dir_ent;
        DIR *dir = opendir(dir_name);

        char* file_path;

        work_param_struct* wp;

        send_param_struct* sp;

        while( (dir_ent = readdir(dir)) != NULL)
        {
            if(dir_ent->d_type == DT_REG) 
            {
                num_workers++;
            }
        }
        closedir(dir);
        
        dir = opendir(dir_name);
        worker_tid = (pthread_t*)malloc(sizeof(pthread_t) * num_workers);
        worker_attr = (pthread_attr_t*)malloc(sizeof(pthread_attr_t) * num_workers);


        
        
        while((  (dir_ent = readdir(dir)) != NULL && i < num_workers)) // how to propperly assign readdir?
        {
            if(dir_ent->d_type == DT_REG)
            {
                // create file path
                file_path = malloc(sizeof(char) * MAX_LINE_SIZE);
                strcpy(file_path, dir_name);
                strcat(file_path, "/");
                strcat(file_path, dir_ent->d_name);
                // put file path in work_param_struct
                wp = malloc(sizeof(work_param_struct));
                strcpy(wp->f_n, file_path);
                wp->l = buf;
                //create thread
                pthread_create(&worker_tid[i], &worker_attr[i], worker, wp);// work param needs "line + filename"
                i++;
            }

        }

    // start sender thread
        sp = malloc(sizeof(send_param_struct));
        sp->l = buf;
        sp->k = key;
        sp->msg_q_id = message_queue_id;

        pthread_attr_init(&sender_attr);
        pthread_create(&sender_tid, &sender_attr, sender, sp);



// WRAP UP VARIABLES AND POINTERS
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
        free(wp);
        free(sp);
        printf("child process: %d complete\n\n", getpid());
        exit(0);
    }

    //return 0;
}


// FUNCTION DEFINITIONS

void *sender(void *send_param_voidptr)
{
    printf("sender thread %ld starting\n\n", pthread_self());

    send_param_struct* sp = (send_param_struct *) send_param_voidptr;
    printf("key: %d\n\n", sp->k);

    while(sp->l->size != 0) // this is dum, replace with way to check if other threads are done or not
    {
    // while(1)
        // for(worker_threads)
            // if (thread isn't done) flag = false
        // if (flag && l->list->size == 0) exit(0)

        sem_wait(&full);
        sem_wait(&mutex);
        node* m = list_rem_head(sp->l);
        sem_post(&mutex);
        sem_post(&empty);


        if(msgsnd(sp->msg_q_id, &(m->to_send) , MAX_WORD_SIZE, 0) == -1) 
        {
            perror("Error in msgsnd");
        }
        printf("sending {%s}\n\n", m->to_send.content);
        printf("sizeof list: %d\n\n", sp->l->size);
        int i; 
        sem_getvalue(&mutex, &i);
        printf("sem mutex: %i\n", i);
        i = sem_getvalue(&full, &i);
        printf("sem full: %i\n",i);
        i = sem_getvalue(&empty, &i);
        printf("sem empty: %i\n",i);
        free(m);
    }

    

    printf("sender thread %ld ending\n\n", pthread_self());
    pthread_exit(0);
}

void *worker(void *work_param_voidptr)
{
    
    printf("worker thread %ld starting\n\n", pthread_self());



    work_param_struct* wp = (work_param_struct *) work_param_voidptr;



    printf("thread %ld file_name: %s\n\n", pthread_self(), wp->f_n);
    FILE* to_work = fopen(wp->f_n, "r");

    

    free(wp->f_n);

    char word_1[MAX_WORD_SIZE];
    while(fscanf(to_work, "%s", word_1) != EOF)
    {
        
        node* new = create_node(word_1);

        
        sem_wait(&empty);
        sem_wait(&mutex);
        list_add_tail(wp->l, new);
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

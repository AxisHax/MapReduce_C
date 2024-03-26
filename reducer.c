#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ctype.h>

#define MAXWORDSIZE 256

typedef struct message_s
{
	long type;
	char content[MAXWORDSIZE];
} Message;

typedef struct Node 
{
	char *word;
	int count;
	struct Node *next;
	struct Node *prev;
} Node;

typedef struct List 
{
	struct Node *head;
	struct Node *tail;
	int count;
} List;

/// @brief Create a node using the passed in string as its value.
struct Node *node_create(char *word) 
{
	Node *node = malloc(sizeof(Node));
	if (node == NULL) 
	{
		fprintf (stderr, "%s: Couldn't create memory for the node; %s\n", "linkedlist", strerror(errno));
		exit(-1);
	}
	node->word = strdup(word);
	node->count = 1;
	node->next = NULL;
	node->prev = NULL;
	return node;
}

/// @brief Create a new linked list with initial values (head = NULL, tail = NULL, count = 0).
struct List *list_create() 
{
	List *list = malloc(sizeof(List));
	if (list == NULL) 
	{
		fprintf (stderr, "%s: Couldn't create memory for the list; %s\n", "linkedlist", strerror (errno));
		exit(-1);
	}
	list->head = NULL;
	list->tail = NULL;
	list->count = 0;
	return list;
}

/// @brief Append a node to the end of a linked list.
/// @param node 
/// @param list 
void list_insert_tail(Node *node, List *list) 
{
	if (list->head == NULL && list->tail == NULL) 
	{
		list->head = node;
		list->tail = node;
	} 
	else 
	{
		list->tail->next = node;
		node->prev = list->tail;
		list->tail = node;
	}
	list->count++;
}

/// @brief Print all elements of a linked list.
/// @param list 
void list_print(List *list) 
{
	Node *ptr = list->head;  
	while (ptr != NULL) 
	{
		printf("%s:%d\n", ptr->word, ptr->count);
		ptr = ptr->next;
	}
	printf("\n");
}

/// @brief Print elements of the linked list with at least minFrequency count of words.
/// @param list 
/// @param minFrequency 
void list_print_by_frequency(List *list, int minFrequency) 
{
	Node *ptr = list->head;  
	while (ptr != NULL) 
	{
		if(ptr->count >= minFrequency)
		{
			printf("%s:%d\n", ptr->word, ptr->count);
			ptr = ptr->next;
		}
	}
	printf("\n");
}

/// @brief Print the first n elements of a linked list.
/// @param list 
/// @param n 
void list_print_first_n_elements(List *list, int n)
{
	Node *ptr = list->head;
	while(ptr != NULL && n > 0)
	{
		printf("%s:%d\n", ptr->word, ptr->count);
		ptr = ptr->next;
		n--;
	}
	printf("\n");
}

/// @brief Delete a linked list and free its memory.
/// @param list 
void list_destroy(List *list) 
{
	Node *ptr = list->head;
	Node *tmp;  
	while (ptr != NULL) 
	{
		free(ptr->word);
		tmp = ptr;
		ptr = ptr->next;
		free(tmp);
	}
	free(list);
}

/// @brief Insert a node into the list in sorted order. Sorting is based on word count and alphabetically in the case two words have the same count.
/// @param node 
/// @param list 
void list_insert_sorted(Node *node, List *list) {
	Node *pointer = list -> head;
	if(list->head == NULL && list->tail == NULL)
	{// The list is empty, so just add node to the list and return.
		list -> head = node;
		list -> tail = node;
		list -> count++;
		return;
	}
	
	while(pointer != NULL)
	{
		if(node->count > pointer->count)
		{
			if(pointer == list->head)
			{// Insert node at the beginning
				node -> prev = NULL;
				node -> next = pointer;
				pointer -> prev = node;
				list -> head = node;
				list -> count++;
				return;
			}
			else
			{// Insert node between other nodes
				node -> prev = pointer -> prev;
				node -> next = pointer;
				pointer -> prev -> next = node;
				pointer -> prev = node;
				list -> count++;
				return;
			}
		}
		else if(node->count == pointer->count)
		{
			// Sort alphabetically.
			if(strcmp(node->word, pointer->word) < 0)
			{// strcmp returns < 0 when the first unique character in arg1 has a lower ascii value than arg2. Lower ascii value = that word should be first.
				if(pointer == list->head)
				{// Insert node at the beginning
					node -> prev = NULL;
					node -> next = pointer;
					pointer -> prev = node;
					list -> head = node;
					list -> count++;
					return;
				}
				else
				{// Insert node between other nodes
					node -> prev = pointer -> prev;
					node -> next = pointer;
					pointer -> prev -> next = node;
					pointer -> prev = node;
					list -> count++;
					return;
				}
			}
			else
			{// If the word is >= the word we're comparing against, break out. This will allow the pointer to advance and we can check the next node.
				break;
			}
			/* else if(strcmp(node->word, pointer->word) > 0)
			{// If we get a return val greater than 0 then it means the first word has a higher value than the second word, meaning it should be appended after it in the list.
				if(pointer == list->tail)
				{// Insert then update the tail node.
					node -> prev = pointer;
					node -> next = NULL;
					pointer -> next = node;
					list -> tail = node;
					list -> count++;
					return;
				}
				else
				{// We're inserting between two nodes.
					node -> prev = pointer;
					node -> next = pointer -> next;
					pointer -> next -> prev = node;
					pointer -> next = node;
					list -> count++;
					return;
				}
			} */
		}
		else if(pointer == list->tail)
		{// Insert at the end of the list
			pointer -> next = node;
			node -> prev = pointer;
			node -> next = NULL;
			list -> tail = node;
			list -> count++;
			return;
		}
		pointer = pointer -> next;
	}
}

int main(int argc, char* argv[])
{
	if (argc != 3) 
    {
        printf("ERROR: wrong arguments\n");
        return -1;
    } // if file is run without commandFile bufferSize

	//char *outputFile;
	unsigned int minFrequency = atoi(argv[2]);
	//strcpy(*outputFile, argv[1]);

	Message message;
	List *list = list_create();
	Node *node = NULL;
	struct msqid_ds info;
	int message_queue_id;
	key_t key;
	
	if((key = ftok("mapper.c", 1)) == -1)
	{
		perror("ftok");
		exit(1);
	}
	if((message_queue_id = msgget(key, 0444)) == -1)
	{
		perror("msgget");
		exit(1);
	}
	if(msgctl(message_queue_id, IPC_STAT, &info) == -1)
	{
		perror("msgctl");
		exit(1);
	}

	char *end_msg;
	// Do stuff with words in the message queue.
	while(1)// check type of message?
	{
		printf("message queue size: %d\n", (int)info.msg_qnum);
		// Get messages from the message queue
		if(msgrcv(message_queue_id, &message, MAXWORDSIZE, 0, 0) == -1)
		{
			perror("msgrcv");
			exit(1);
		}

		strdup(end_msg, message.content);
		if(strcmp(end_msg, ""))
		{
			free(end_msg);
			break;
		}

		printf("%s\n", message.content);
		// Check if the word already exists in the list. If not, simply add, but if so just increment the count.
		if(list->head == NULL && list->tail == NULL)
		{
			node = node_create(message.content);
			list_insert_tail(node, list);
		}
		else
		{
			Node *cur = list->head;
			int wordExists = 0; // 0 = false, 1 = true.
			
			while(cur != NULL)
			{
				if(strcasecmp(message.content, cur->word) == 0)
				{
					wordExists = 1; // The word exists in our list.
				}
			}
			free(cur);
			
			if(wordExists == 0)
			{// If we didn't find our word in the list, add it.
				node = node_create(message.content);
				list_insert_tail(node, list);
			}
		}
	}

	// Now add the words into a new list but in sorted order
	List *sortedWords = list_create();
	Node *cur = list->head;
	while(cur != NULL)
		list_insert_sorted(cur, sortedWords);
	
	// Print list contents.
	list_print_by_frequency(sortedWords, minFrequency);
	
	if(msgctl(message_queue_id, IPC_RMID, NULL) == -1)
	{
		perror("msgctl");
		exit(1);
	}

	list_destroy(list);
	list_destroy(sortedWords);
	free(cur);
	free(node);

	return 0;
}

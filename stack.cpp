
/*
 * File:    stack.c
 * Author:  zentut.com
 * Purpose: linked stack implementation
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.hpp"
 

/*
    stack_init the stack
*/
void stack_init(struct stack_node* head)
{
    head = NULL;
}
 
/*
    push an element into stack
*/
struct stack_node* push(struct stack_node* head,char * data)
{
    struct stack_node* tmp = (struct stack_node*)malloc(sizeof(struct stack_node));
    if(tmp == NULL)
    {
        exit(0);
    }
    tmp->data = (char *)malloc(sizeof(char)*(strlen(data)+1));
    strcpy(tmp->data, data);
    tmp->next = head;
    head = tmp;
    return head;
}
/*
    pop an element from the stack
*/
struct stack_node* pop(struct stack_node *head)
{
    struct stack_node* tmp = head;
    head = head->next;
    free(tmp->data);
    free(tmp);
    return head;
}

/*
    returns 1 if the stack is empty, otherwise returns 0
*/

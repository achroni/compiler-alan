/*
 * File:    stack.h
 * Author:  zentut.com
 * Purpose: linked stack header file
 */
#ifndef LINKEDSTACK_H_INCLUDED
#define LINKEDSTACK_H_INCLUDED



struct stack_node
{
    char * data;
    struct stack_node* next;
};
 
typedef struct stack_node * Stack;

extern Stack rootFunc;	

struct stack_node* push(struct stack_node *s,char* data);
struct stack_node* pop(struct stack_node *s);
void stack_init(struct stack_node* s);

#endif // LINKEDSTACK_H_INCLUDED
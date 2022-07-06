/*
    CodeWeavers Programming Test -- Code Review

    The task:  review the enclosed code and find
        1)  Design flaws
        2)  Programming errors
        3)  Style problems

    Review the code for the above three criteria, marking the code
    up visibly (the markings don't have to compile).

    For design and style flaws, you should breifly describe the flaw
    and suggest a solution (it is not necessary to code a full solution).
    If a design or style flaw is repeated, you do not need to discuss it
    again.

    Please suggest fixes for any bugs that would cause the code to fail
    from running as it is currently designed.

    Note:  This code was _intentionally_ written to be bad, you
    should expect many problems.
*/

/* Volker:
    -   I would recommend every programmer to configure their editor to run
            clang-format when saving a file. This way
            - issue#1 could be spotted more easily
            - issue#2 would not occur
            - the programmer would have saved time
        (Disclosure: I found issue#1 and issue#2 by looking at the clang-format diff.)
    -   I would recommend compiling everything with -Wall -Wextra or similar
        options. Some mistakes, like issue#1 would have produced a warning.
    -   Some style guides recommend never doing this:
            if (cond)
                expr;
        and instead always doing this:
            if (cond) {
                expr;
            }
        Google "goto fail bug" for an important example.
    -   (Potential) design flaw: Linked lists are nearly always slower than
        arrays. Don't use them except for very rare cases.
    -   There are no unit tests.
    -   (Potential) design flaw: You should rarely implement something that
        smart people have implemented before. Instead of writing this, you could
        have used an existing linked-list library.
*/


/*  Linked list management subroutine library */
// Volker: calling a struct `n` is bad style, give it some longer, descriptive
// name like `linked_list`
struct  n
{
    void        *user_data;
    // Volker:
    // - calling a variable `f` is bad style, give it some longer, descriptive
    //   name like `free_flag`
    // - This should be a bool, not an integer
    // - I think that a struct that contains an integer that indicates that it
    //   should be free'd is a weird design choice. Never saw something like
    //   that. Seems like a waste of memory.
    int         f;
    struct n    *next;
};

// Volker: I thought about whether the linked list below might be slightly
// faster, since you do not need to do calculate pointer+16 if you do something
// like
// for (p = head; p; p = p->next)
// But integer addition is fast and chasing pointers is slow, so I think it is
// negligible.
struct  n
{
    struct n    *next;
    void        *user_data;
    int         f;
};



/* Insert at end of list */
void insert(struct n *head, void *data, int data_size)
{
    struct n *new_node, *p;

    new_node = malloc(sizeof(new_node));
    // Volker: Should me sizeof(n) instead of sizeof(new_node). sizeof(new_node)
    // would be the size of a pointer, usually 8 Bytes.
    // Volker: missing assert(new_node != NULL);

    // Volker: new_node->f is not initialized in this function. Add
    //`new_node->f = 0;` here. Note that reading uninitialised memory not just
    //gives you garbage, it's actually UB.

    new_node->next = NULL;

    // Volker: `new_node = data;` would be faster than allocating and copying.
    // But it would require changing the code of the caller.
    new_node->user_data = malloc(data_size);
    // Volker: missing assert(new_node->user_data != NULL);
    // Volker: use data_size instead of sizeof(data). sizeof(data) would be the
    // size of a pointer, usually 8 Bytes.
    memcpy(new_node->user_data, data, sizeof(data));

    /* If list is empty, just insert at the top */
    if (head == NULL)
        head = new_node;
    else
        /*  Else, insert at end of list */
        for (p = head; p; p = p->next)
            /* We're at the end if the next node is NULL, so insert */
            if (p->next == NULL)
                p->next = new_node;
}



/* Traverse the list, invoking a passed in callback function, which
    can optionally tell us to free the node when we return  */
void traverse(struct n *head,
        void (*callback_func)(void *user_data, int free_flag))
{
    // Volker issue#2: The whitespace and commenting in this function is
    // incoherent with the rest. It should be:
//    struct n *p;
//
//    for (p = head; p; p = p->next)
//    {
//        (*callback_func)(p->user_data, p->f);
//        /* If they want to free the node, do that */
//        if (p->f)
//            free(p);
//    }
  struct n *p;

  for (p = head; p; p = p->next)    {
    (*callback_func)(p->user_data, p->f);
    if (p->f)           // If they want to free the node, do that
        free(p);
  }
    // Volker:
    // 3 problems with this code:
    // -    It free's p, but not p->user_data
    // -    You cannot remove an element from the middle of a linked list by
    //      freeing it. You need to reconnect the two parts of the chain.
    // -    Let's say p->f is 0 and callback func is e.g.
    //      void my_callback_func(void *user_data, int free_flag) {
    //          free_flag = 1;
    //      }
    //      After `(*callback_func)(p->user_data, p->f)` the value of `p->f`
    //      would still be zero. You need to either
    //      -   Use pass-by-reference
    //      -   Let callback_func return a return value that indicates whether
    //          `p` should be freed.

}

// Volker: Fixed version of `traverse`
void traverse(struct n *head,
        bool (*callback_func)(void *user_data))
{
    struct n **ptr = &head;
    while((*ptr))
    {
        if ((*callback_func)((*ptr)->user_data))
        {
            struct n *next = (*ptr)->next;
            free((*ptr)->user_data);
            free(*ptr);
            *ptr = next;
        }
        else
        {
            ptr = &((*ptr)->next);
        }
    }
}



/* Free the list */
// Volker: Don't call it freel, call it free_list. Otherwise, it looks to
// similar to free
void free_list(struct n *head)
{
    struct n *p;
    for (p = head; p; p = p->next)
        free(p);
    // Volker: There are two problems with this function:
    //  - It only free's the `new_node = malloc(sizeof(new_node))` allocation,
    //    but does not free the `new_node->user_data = malloc(data_size);`
    //    allocation
    // - It frees the list in the wrong order: It does this:
    // p1 = head
    // free(p1)
    // p2 = p1->next
    // free(p2)
    // p3 = p2->next
    // free(p3)
    // ...
    // The problem is that `p2 = p1->next` is UB, because it dereferences p1,
    // which was freed in the line above.
    // This is a use-after-free bug.
    // If you run this with -O0, it will probably work, because the use is
    // immediately after the free, so you probably still find the correct values
    // in the deallocated memory region. Once you add -O2 the compiler will
    // probably see that this function is UB if head is not NULL and funny
    // things might happen.
    //
    // Fixed version:
    if(head == NULL)
    {
        return;
    }
    // Volker: Note that this recursion might produce a stack overflow if the
    // list is too long.
    free_list(head->next);
    free(head->user_data);
    free(head);
}


/* Quickly sort the list */
void qsort(struct n *head, int compare(void *a, void *b))
{
    struct n *p, *q;

    // Volker issue#1:
    // There is a semicolon after the for loop. This is equivalent to
    // for (p = head; p; p = p->next) {}
    // Therfore, p->next would be a null pointer dereference.
    // You should remove that semicolon
    for (p = head; p; p = p->next);
        for (q = p->next; q; q = q->next)
        {
            if (compare(q->user_data, p->user_data) < 0)
            {
                /* Swap 'em */ // Volker: This is not a swap, it sets both to q->user_data
                p->user_data = q->user_data;
                q->user_data = p->user_data;
            }
        }


    // Volker: Corrected version
    for (p = head; p; p = p->next)
    {
        for (q = p->next; q; q = q->next)
        {
            if (compare(q->user_data, p->user_data) < 0)
            {
                /* Swap 'em */
                void *temp = p->user_data;
                p->user_data = q->user_data;
                q->user_data = temp;
            }
        }
    }
}

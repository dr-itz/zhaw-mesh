#ifndef LIB_LIST_H
#define LIB_LIST_H

/**
 * @file list.h
 * @brief
 * doubly linked list implementation, mostly from the Linux kernel list.h,
 * with some code from ccodearchive.net and some by D.Ritz.
 */

typedef struct list_head list_head_t;
struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	list_head_t name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(list_head_t *list)
{
	list->prev = list;
	list->next = list;
}

/*
 * Insert a entry entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(list_head_t *entry, list_head_t *prev, list_head_t *next)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

/**
 * list_add - add a entry entry
 * @param entry entry entry to be added
 * @param head list head to add it after
 *
 * Insert a entry entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(list_head_t *entry, list_head_t *head)
{
	__list_add(entry, head, head->next);
}

/**
 * list_add_tail - add a entry entry
 * @param entry entry entry to be added
 * @param head list head to add it before
 *
 * Insert a entry entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(list_head_t *entry, list_head_t *head)
{
	__list_add(entry, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_remove(list_head_t *prev, list_head_t *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_remove - deletes entry from list.
 * @param entry the element to delete from the list.
 */
static inline void list_remove(list_head_t *entry)
{
	__list_remove(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}


/**
 * list_remove_init - deletes entry from list and reinitialize it.
 * @param entry the element to delete from the list.
 */
static inline void list_remove_init(list_head_t *entry)
{
	__list_remove(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @param entry the entry to move
 * @param head the head that will precede our entry
 */
static inline void list_move(list_head_t *entry, list_head_t *head)
{
	__list_remove(entry->prev, entry->next);
	list_add(entry, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @param entry the entry to move
 * @param head the head that will follow our entry
 */
static inline void list_move_tail(list_head_t *entry, list_head_t *head)
{
	__list_remove(entry->prev, entry->next);
	list_add_tail(entry, head);
}

/**
 * list_empty - tests whether a list is empty
 * @param head the list to test.
 */
static inline int list_empty(list_head_t *head)
{
	return head->next == head;
}


/**
 * Returns true if there is a next element
 * @param entry The list element
 * @param head The list head
 */
static inline int list_has_next(list_head_t *entry, list_head_t *head)
{
	return entry->next != head;
}

/**
 * Returns true if there is a prev element
 * @param entry The list element
 * @param head The list head
 */
static inline int list_has_prev(list_head_t *entry, list_head_t *head)
{
	return entry->prev != head;
}


/* offset of a member in a struct */
#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)NULL)->member)
#endif

/* returns pointer to struct begin, when ptr points to member in struct of type */
#ifdef __GNUC__
#define container_of(ptr, type, member) ({               \
	const __typeof__(((type *) 0)->member) *__mptr = (ptr);  \
	(type *) ((char *)__mptr - offsetof(type, member));})
#define container_of_var(ptr, container_var, member) \
	container_of(ptr, __typeof__(*container_var), member)

#else /* !__GNUC__ i.e. no __typeof__ */
#define container_of(ptr, type, member) \
	((type *) ((char *)(ptr) - offsetof(type, member)))
#define container_off_var(var, member) \
	((char *) &(var)->member - (char *) (var))
#define container_of_var(ptr, container_var, member) \
	((void *) ((char *)(ptr) - container_off_var(container_var, member)))

#endif /* __GNUC__ */

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_for_each - iterate over a list
 * @param pos	the &list_head_t to use as a loop counter.
 * @param head	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @param pos	the &list_head_t to use as a loop counter.
 * @param n	another &list_head_t to use as temporary storage
 * @param head	the head for your list.
 */
#define list_for_each_safe(pos, n, head)                   \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_entry - iterate over list of given type
 * @param pos        the type * to use as a loop counter.
 * @param head       the head for your list.
 * @param member     the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)                    \
	for (pos = NULL, pos = container_of_var((head)->next, pos, member);      \
	     &pos->member != (head);                                  \
	     pos = container_of_var(pos->member.next, pos, member))


/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @param pos        the type * to use as a loop counter.
 * @param n          another type * to use as temporary storage
 * @param head       the head for your list.
 * @param member     the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)             \
	for (pos = NULL, pos = container_of_var((head)->next, pos, member),       \
	       n = container_of_var(pos->member.next, pos, member);   \
	    &pos->member != (head);                                    \
	    pos = n, n = container_of_var(n->member.next, n, member))


/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @param pos        the type * to use as a loop counter.
 * @param head       the head for your list.
 * @param member     the name of the list_struct within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)            \
	for (pos = NULL, pos = container_of_var((head)->prev, pos, member);      \
	     &pos->member != (head);                                  \
	     pos = container_of_var(pos->member.prev, pos, member))

#endif /* LIB_LIST_H */

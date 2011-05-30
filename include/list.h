#ifndef __LIST_H
#define __LIST_H

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

#define LIST_HOLE ((struct list_head *)0xfffffff)

static inline int list_empty(struct list_head *list)
{
	return (list->prev == list) && (list->next == list);
}

static inline void list_init(struct list_head *head)
{
	head->prev = head;
	head->next = head;
}

static inline void __list_add(struct list_head *head,
				struct list_head *prev,
				struct list_head *next)
{
	head->prev = prev;
	head->next = next;
	next->prev = head;
	prev->next = head;
}

static inline void list_add(struct list_head *list,
			struct list_head *head)
{
	__list_add(list, head, head->next);
}

static inline void list_add_tail(struct list_head *list,
			struct list_head *head)
{
	__list_add(list, head->prev, head);
}

static inline void __list_del(struct list_head *prev,
				struct list_head *next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void list_del(struct list_head *list)
{
	__list_del(list->prev, list->next);
	list->prev = LIST_HOLE;
	list->next = LIST_HOLE;
}

#define list_entry(ptr, type, member)\
	 ((type *)((void *)ptr - (int)(&((type *)0)->member)))

#define list_for_each_entry(pos, head, member)\
	for (pos = list_entry((head)->next, typeof(*pos), member);\
		&pos->member != (head);\
		pos = list_entry(pos->member.next, typeof(*pos), member))

#endif	/* list.h */

#ifndef DS
#define DS
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#define LIST_INIT_CAP 256

#define List(Type)                                                             \
	struct {                                                                     \
		size_t count;                                                              \
		size_t capacity;                                                           \
		Type* items;                                                               \
	}

#define list_init(list, cap)                                                   \
	do {                                                                         \
		(list)->items = malloc(sizeof(*(list)->items) * cap);                      \
		assert((list)->items != NULL && "Buy more RAM!!");                         \
		(list)->count = 0;                                                         \
		(list)->capacity = (cap);                                                  \
	} while (0)

#define list_append(list, item)                                                \
	do {                                                                         \
		if ((list)->count >= (list)->capacity) {                                   \
			(list)->capacity                                                         \
					= ((list)->capacity) == 0 ? LIST_INIT_CAP : (list)->capacity * 2;    \
			void* tmp = realloc(                                                     \
					(list)->items, (list)->capacity * sizeof(*((list)->items)));         \
			assert(tmp != NULL && "Buy more RAM!!");                                 \
			(list)->items = tmp;                                                     \
		}                                                                          \
		(list)->items[(list)->count++] = (item);                                   \
	} while (0)

#define list_clear(list)                                                       \
	do {                                                                         \
		(list)->count = 0;                                                         \
	} while (0)

#endif // !DS

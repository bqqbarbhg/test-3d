
struct Temp_Allocation
{
	Temp_Allocation *prev;
	Temp_Allocation *next;
	Temp_Allocation *parent;
	void **pointer;
	char *temp_data;
	char *allocated_data;
	size_t size;
	size_t alignment;
};

struct Temp_Allocator
{
	Temp_Allocation *head, *tail;
};

#define TEMP_ALLOC_TEMP(alloc, type, count) (type*)calloc(count, sizeof(type))

Temp_Allocation *temp_allocator_add(Temp_Allocator *allocator, void **pointer,
		size_t size, size_t alignment)
{
	Temp_Allocation *a = TEMP_ALLOC_TEMP(allocator, Temp_Allocation, 1);
	if (!a) return 0;

	a->pointer = pointer;
	a->size = size;
	a->alignment = alignment;
	a->temp_data = TEMP_ALLOC_TEMP(allocator, char, size * alignment);
	a->allocated_data = 0;
	a->next = 0;

	if (!allocator->tail)
		allocator->tail = a;

	a->prev = allocator->head;
	if (a->prev)
		a->prev->next = a;
	allocator->head = a;

	a->parent = 0;

	for (Temp_Allocation *prev = a->prev; prev; prev = prev->prev) {
		ptrdiff_t diff = a->temp_data - prev->temp_data;

		if (diff >= 0 && diff < (ptrdiff_t)prev->size) {
			a->parent = prev;
			break;
		}
	}

	*pointer = a->temp_data;
	return a;
}

Temp_Allocation *temp_allocator_copy(Temp_Allocator *allocator, void **pointer,
		size_t size, size_t alignment, const void *data)
{
	Temp_Allocation *a = temp_allocator_add(allocator, pointer, size, alignment);
	if (!a) return 0;

	memcpy(a->temp_data, data, size);
	return a;
}

void temp_allocator_free(Temp_Allocator *allocator)
{
	Temp_Allocation *next = 0;
	for (Temp_Allocation *a = allocator->tail; a; a = next) {
		next = a->next;
		free(a);
	}

	allocator->head = 0;
	allocator->tail = 0;
}

void *temp_allocator_finalize(Temp_Allocator *allocator)
{
	size_t total_size = 0;

	for (Temp_Allocation *a = allocator->tail; a; a = a->next) {
		size_t alignment = (a->alignment - total_size % a->alignment) % a->alignment;
		total_size += alignment;
		total_size += a->size;
	}

	char *allocation = (char*)malloc(total_size);
	if (!allocation) {
		temp_allocator_free(allocator);
		return 0;
	}

	size_t position = 0;

	for (Temp_Allocation *a = allocator->tail; a; a = a->next) {
		size_t alignment = (a->alignment - total_size % a->alignment) % a->alignment;
		position += alignment;

		a->allocated_data = allocation + position;
		position += a->size;
	}

	for (Temp_Allocation *a = allocator->tail; a; a = a->next) {
		Temp_Allocation *p = a->parent;
		if (!p)
			continue;

		a->pointer = (void**)(p->allocated_data + ((char*)a->pointer - p->temp_data));
	}

	for (Temp_Allocation *a = allocator->tail; a; a = a->next) {
		Temp_Allocation *p = a->parent;
		if (!p)
			continue;

		*a->pointer = a->allocated_data;
	}

	temp_allocator_free(allocator);
	return allocation;
}

#define TEMP_ALLOC_N(allocator, pointer, count) (temp_allocator_add((allocator), (void**)&(pointer), sizeof(*(pointer)) * (count), 8))

#define TEMP_COPY_STR(allocator, pointer, data) (temp_allocator_copy((allocator), (void**)&(pointer), strlen(data) + 1, 1, data))


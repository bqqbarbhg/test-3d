
struct Out_Stream
{
	char *data;
	size_t size, capacity;
};

struct In_Stream
{
	char *data;
	size_t pos, size;
};

void stream_write(Out_Stream *s, void *data, size_t size, size_t count = 1)
{
	size_t bytes = size * count;
	size_t new_size = s->size + bytes;

	if (new_size > s->capacity) {
		size_t new_capacity = new_size * 2;
		s->data = (char*)realloc(s->data, new_capacity);
		s->capacity = new_capacity;
	}

	memcpy(s->data + s->size, data, bytes);
	s->size = new_size;
}

void stream_free(Out_Stream *s)
{
	free(s->data);
	s->data = 0;
}

void stream_read(In_Stream *s, void *data, size_t size, size_t count = 1)
{
	size_t bytes = size * count;
	size_t new_pos = s->pos + bytes;

	assert(new_pos <= s->size);

	memcpy(data, s->data + s->pos, bytes);
	s->pos = new_pos;
}

void *stream_skip(In_Stream *s, size_t size, size_t count = 1)
{
	size_t bytes = size * count;
	size_t new_pos = s->pos + bytes;

	assert(new_pos <= s->size);

	void *ptr = s->data + s->pos;
	s->pos = new_pos;

	return ptr;
}

In_Stream in_stream(void *data, size_t size)
{
	In_Stream ret;
	ret.data = (char*)data;
	ret.pos = 0;
	ret.size = size;
	return ret;
}


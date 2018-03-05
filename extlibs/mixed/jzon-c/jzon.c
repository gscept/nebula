#include "jzon.h"
#include <stdlib.h>
#include <string.h>

#if !_MSC_VER
#define __forceinline
#endif


// String helpers

char* copy_str(JzonAllocator* allocator, const char* str, unsigned len)
{
	unsigned size = len + 1;
	char* new_str = (char*)allocator->allocate(size);
	memcpy(new_str, str, len);
	new_str[len] = '\0';
	return new_str;
}

bool str_equals(char* str, char* other)
{
	return strcmp(str, other) == 0;
}

char* concat_str(JzonAllocator* allocator, const char* str1, unsigned str1_len, const char* str2, unsigned str2_len)
{
	unsigned size = str1_len + str2_len;
	char* new_str = (char*)allocator->allocate(size + 1);
	memcpy(new_str, str1, str1_len);
	memcpy(new_str + str1_len, str2, str2_len);
	new_str[size] = '\0';
	return new_str;
}

// Array helpers

typedef struct Array
{
	int size;
	int capacity;
	void** arr;
} Array;

void arr_grow(Array* arr, JzonAllocator* allocator)
{
	int new_capacity = arr->capacity == 0 ? 1 : arr->capacity * 2;
	void** new_arr = (void**)allocator->allocate(new_capacity * sizeof(void**));
	memcpy(new_arr, arr->arr, arr->size * sizeof(void**));
	allocator->deallocate(arr->arr);
	arr->arr = new_arr;
	arr->capacity = new_capacity;
}

void arr_add(Array* arr, void* e, JzonAllocator* allocator)
{
	if (arr->size == arr->capacity)
		arr_grow(arr, allocator);

	arr->arr[arr->size] = e;
	++arr->size;
}

void arr_insert(Array* arr, void* e, unsigned index, JzonAllocator* allocator)
{
	if (arr->size == arr->capacity)
		arr_grow(arr, allocator);

	memmove(arr->arr + index + 1, arr->arr + index, (arr->size - index) * sizeof(void**));
	arr->arr[index] = e;
	++arr->size;
}


// Hash function used for hashing object keys.
// From http://murmurhash.googlepages.com/

uint64_t hash_str(const char* str)
{
	size_t len = strlen(str);
	uint64_t seed = 0;

	const uint64_t m = 0xc6a4a7935bd1e995ULL;
	const uint32_t r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t * data = (const uint64_t *)str;
	const uint64_t * end = data + (len / 8);

	while (data != end)
	{
#ifdef PLATFORM_BIG_ENDIAN
		uint64_t k = *data++;
		char *p = (char *)&k;
		char c;
		c = p[0]; p[0] = p[7]; p[7] = c;
		c = p[1]; p[1] = p[6]; p[6] = c;
		c = p[2]; p[2] = p[5]; p[5] = c;
		c = p[3]; p[3] = p[4]; p[4] = c;
#else
		uint64_t k = *data++;
#endif

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch (len & 7)
	{
	case 7: h ^= ((uint64_t)data2[6]) << 48;
	case 6: h ^= ((uint64_t)data2[5]) << 40;
	case 5: h ^= ((uint64_t)data2[4]) << 32;
	case 4: h ^= ((uint64_t)data2[3]) << 24;
	case 3: h ^= ((uint64_t)data2[2]) << 16;
	case 2: h ^= ((uint64_t)data2[1]) << 8;
	case 1: h ^= ((uint64_t)data2[0]);
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}


// Jzon implementation

__forceinline void next(const char** input)
{
	++*input;
}

__forceinline char current(const char** input)
{
	return **input;
}

bool is_multiline_string_quotes(const char* str)
{
	return *str == '"' && *(str + 1) == '"' && *(str + 1) == '"';
}

int find_object_pair_insertion_index(JzonKeyValuePair** objects, unsigned size, uint64_t key_hash)
{
	if (size == 0)
		return 0;

	for (unsigned i = 0; i < size; ++i)
	{
		if (objects[i]->key_hash > key_hash)
			return i;
	}

	return size;
}

void skip_whitespace(const char** input)
{
	while (current(input))
	{
		while (current(input) && (current(input) <= ' ' || current(input) == ','))
			next(input);
		
		// Skip comment.
		if (current(input) == '#')
		{
			while (current(input) && current(input) != '\n')
				next(input);
		}
		else
			break;
	}
};

char* parse_multiline_string(const char** input, JzonAllocator* allocator)
{
	if (!is_multiline_string_quotes(*input))
		return NULL;
	
	*input += 3;
	char* start = (char*)*input;
	char* result = "";

	while (current(input))
	{
		if (current(input) == '\n' || current(input) == '\r')
		{
			unsigned result_len = (unsigned)strlen(result);
			unsigned line_len = (unsigned)(*input - start);

			if (result_len > 0) {
				char* new_result = concat_str(allocator, result, result_len, "\n", 1);
				allocator->deallocate(result);
				result = new_result;
				++result_len;
			}

			skip_whitespace(input);

			if (line_len != 0)
			{
				char* new_result = concat_str(allocator, result, result_len, start, line_len);

				if (result_len > 0)
					allocator->deallocate(result);

				result = new_result;
			}

			start = (char*)*input;
		}

		if (is_multiline_string_quotes(*input))
		{
			unsigned result_len = (unsigned)strlen(result);
			char* new_result = concat_str(allocator, result, result_len, start, (unsigned)(*input - start));
			allocator->deallocate(result);
			result = new_result;
			*input += 3;
			return result;
		}

		next(input);
	}

	allocator->deallocate(result);
	return NULL;
}

char* parse_string_internal(const char** input, JzonAllocator* allocator)
{
	if (current(input) != '"')
		return NULL;

	if (is_multiline_string_quotes(*input))
		return parse_multiline_string(input, allocator);

	next(input);
	char* start = (char*)*input;

	while (current(input))
	{
		if (current(input) == '"')
		{
			char* end = (char*)*input;
			next(input);
			return copy_str(allocator, start, (unsigned)(end - start));
			break;
		}

		next(input);
	}

	return NULL;
}

char* parse_keyname(const char** input, JzonAllocator* allocator)
{
	if (current(input) == '"')
		return parse_string_internal(input, allocator);

	char* start = (char*)*input;

	while (current(input))
	{
		if (current(input) == ':')
			return copy_str(allocator, start, (unsigned)(*input - start));

		next(input);
	}

	return NULL;
}

int parse_value(const char** input, JzonValue* output, JzonAllocator* allocator);

int parse_string(const char** input, JzonValue* output, JzonAllocator* allocator)
{
	char* str = parse_string_internal(input, allocator);

	if (str == NULL)
		return -1;

	output->is_string = true;
	output->string_value = str;
	return 0;
}

int parse_array(const char** input, JzonValue* output, JzonAllocator* allocator)
{	
	if (current(input) != '[')
		return -1;
	
	output->is_array = true;
	next(input);

	// Empty array.
	if (current(input) == ']')
	{
		next(input);
		output->size = 0; 
		return 0;
	}

	Array array_values = { 0 };

	while (current(input))
	{
		skip_whitespace(input);
		JzonValue* value = (JzonValue*)allocator->allocate(sizeof(JzonValue));
		memset(value, 0, sizeof(JzonValue));
		int error = parse_value(input, value, allocator);

		if (error != 0)
			return error;

		arr_add(&array_values, value, allocator);
		skip_whitespace(input);

		if (current(input) == ']')
		{
			next(input);
			break;
		}
	}
	
	output->size = array_values.size; 
	output->array_values = (JzonValue**)array_values.arr;	
	return 0;
}

int parse_object(const char** input, JzonValue* output, bool root_object, JzonAllocator* allocator)
{
	if (current(input) == '{')
next(input);
	else if (!root_object)
		return -1;

		output->is_object = true;

		// Empty object.
		if (current(input) == '}')
		{
			output->size = 0;
			return 0;
		}

		Array object_values = { 0 };
		Array array_values = { 0 };
		while (current(input))
		{
			JzonKeyValuePair* pair = (JzonKeyValuePair*)allocator->allocate(sizeof(JzonKeyValuePair));
			skip_whitespace(input);
			char* key = parse_keyname(input, allocator);
			skip_whitespace(input);

			if (key == NULL || current(input) != ':')
				return -1;

			next(input);
			JzonValue* value = (JzonValue*)allocator->allocate(sizeof(JzonValue));
			memset(value, 0, sizeof(JzonValue));
			value->key = key;
			int error = parse_value(input, value, allocator);

			if (error != 0)
				return error;

			pair->key = key;
			pair->key_hash = hash_str(key);
			pair->value = value;
			arr_add(&array_values, value, allocator);
			arr_insert(&object_values, pair, find_object_pair_insertion_index((JzonKeyValuePair**)object_values.arr, object_values.size, pair->key_hash), allocator);
			skip_whitespace(input);

			if (current(input) == '}')
			{
				next(input);
				break;
			}
		}

		output->size = object_values.size;
		output->object_values = (JzonKeyValuePair**)object_values.arr;
		output->array_values = (JzonValue**)array_values.arr;
		return 0;
}

int parse_number(const char** input, JzonValue* output)
{
	bool is_float = false;
	char* start = (char*)*input;

	if (current(input) == '-')
		next(input);

	while (current(input) >= '0' && current(input) <= '9')
		next(input);

	if (current(input) == '.')
	{
		is_float = true;
		next(input);

		while (current(input) >= '0' && current(input) <= '9')
			next(input);
	}

	if (current(input) == 'e' || current(input) == 'E')
	{
		is_float = true;
		next(input);

		if (current(input) == '-' || current(input) == '+')
			next(input);

		while (current(input) >= '0' && current(input) <= '9')
			next(input);
	}

	if (is_float)
	{
		output->is_float = true;
		output->float_value = (float)strtod(start, NULL);
		output->int_value = (int)output->float_value;
	}
	else
	{
		output->is_int = true;
		output->int_value = (int)strtol(start, NULL, 10);
		output->float_value = (float)output->int_value;
	}

	return 0;
}

int parse_true(const char** input, JzonValue* output)
{
	if (**input == 't' && *((*input) + 1) == 'r' && *((*input) + 2) == 'u' && *((*input) + 3) == 'e')
	{
		next(input); next(input); next(input); next(input);
		output->is_bool = true;
		output->bool_value = true;
		return 0;
	}

	return -1;
}

int parse_false(const char** input, JzonValue* output)
{
	if (**input == 'f' && *((*input) + 1) == 'a' && *((*input) + 2) == 'l' && *((*input) + 3) == 's' && *((*input) + 4) == 'e')
	{
		next(input); next(input); next(input); next(input); next(input);
		output->is_bool = true;
		output->bool_value = false;
		return 0;
	}

	return -1;
}

int parse_null(const char** input, JzonValue* output)
{
	if (**input == 'n' && *((*input) + 1) == 'u' && *((*input) + 2) == 'l' && *((*input) + 3) == 'l')
	{
		next(input); next(input); next(input); next(input); 
		output->is_null = true;
		return 0;
	}

	return -1;
}

int parse_value(const char** input, JzonValue* output, JzonAllocator* allocator)
{
	skip_whitespace(input);
	char ch = current(input);

	switch (ch)
	{
		case '{': return parse_object(input, output, false, allocator);
		case '[': return parse_array(input, output, allocator);
		case '"': return parse_string(input, output, allocator);
		case '-': return parse_number(input, output);
		case 'f': return parse_false(input, output);
		case 't': return parse_true(input, output);
		case 'n': return parse_null(input, output);
		default: return ch >= '0' && ch <= '9' ? parse_number(input, output) : -1;
	}
}


// Public interface

JzonParseResult jzon_parse_custom_allocator(const char* input, JzonAllocator* allocator)
{
	JzonValue* output = (JzonValue*)allocator->allocate(sizeof(JzonValue));
	memset(output, 0, sizeof(JzonValue));
	int error = parse_object(&input, output, true, allocator);
	JzonParseResult result = {0};
	result.output = output;
	result.error = input;
	result.success = error == 0;
	return result;
}

JzonParseResult jzon_parse(const char* input)
{
	JzonAllocator allocator = { malloc, free };
	return jzon_parse_custom_allocator(input, &allocator);
}

void jzon_free_custom_allocator(JzonValue* value, JzonAllocator* allocator)
{
	if (value->is_object)
	{
		for (unsigned i = 0; i < value->size; ++i)
		{
			allocator->deallocate(value->object_values[i]->key);
			jzon_free_custom_allocator(value->object_values[i]->value, allocator);
		}

		allocator->deallocate(value->object_values);
	}
	else if (value->is_array)
	{
		for (unsigned i = 0; i < value->size; ++i)
			jzon_free_custom_allocator(value->array_values[i], allocator);

		allocator->deallocate(value->array_values);
	}
	else if (value->is_string)
	{
		allocator->deallocate(value->string_value);
	}

	allocator->deallocate(value);
}

void jzon_free(JzonValue* value)
{
	JzonAllocator allocator = { malloc, free };
	jzon_free_custom_allocator(value, &allocator);
}

JzonValue* jzon_get(JzonValue* object, const char* key)
{
	if (!object->is_object)
		return NULL;

	if (object->size == 0)
		return NULL;
	
	uint64_t key_hash = hash_str(key);

	int first = 0;
	int last = object->size - 1;
	int middle = (first + last) / 2;

	while (first <= last)
	{
		if (object->object_values[middle]->key_hash < key_hash)
			first = middle + 1;
		else if (object->object_values[middle]->key_hash == key_hash)
			return object->object_values[middle]->value;
		else
			last = middle - 1;

		middle = (first + last) / 2;
	}

	return NULL;
}

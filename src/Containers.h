#ifndef DECLARE_ARRAY
#define DECLARE_ARRAY(_TYPE) \
struct Array_##_TYPE \
{ \
	_TYPE *data; \
	u32 size; \
	u32 capacity_; \
	\
	_TYPE &operator[](int idx) \
	{ \
		ASSERT((u32)idx < capacity_); \
		return data[idx]; \
	} \
	\
	const _TYPE &operator[](int idx) const \
	{ \
		ASSERT((u32)idx < capacity_); \
		return data[idx]; \
	} \
}; \
\
inline void ArrayInit_##_TYPE(Array_##_TYPE *array, u32 capacity, void *(*allocFunc)(u64)) \
{ \
	array->data = (_TYPE *)allocFunc(sizeof(_TYPE) * capacity); \
	array->size = 0; \
	array->capacity_ = capacity; \
}
#endif

#ifndef DECLARE_DYNAMIC_ARRAY
#define DECLARE_DYNAMIC_ARRAY(_TYPE) \
struct DynamicArray_##_TYPE \
{ \
	_TYPE *data; \
	u32 size; \
	u32 capacity; \
	\
	_TYPE &operator[](int idx) \
	{ \
		ASSERT((u32)idx < capacity); \
		return data[idx]; \
	} \
	\
	const _TYPE &operator[](int idx) const \
	{ \
		ASSERT((u32)idx < capacity); \
		return data[idx]; \
	} \
}; \
\
inline void DynamicArrayInit_##_TYPE(DynamicArray_##_TYPE *array, u32 capacity, void *(*allocFunc)(u64)) \
{ \
	array->data = (_TYPE *)allocFunc(sizeof(_TYPE) * capacity); \
	array->size = 0; \
	array->capacity = capacity; \
} \
\
inline u32 DynamicArrayAdd_##_TYPE(DynamicArray_##_TYPE *array, void *(*reallocFunc)(void *, u64)) \
{ \
	if (array->size >= array->capacity) \
	{ \
		array->capacity *= 2; \
		array->data = (_TYPE *)reallocFunc(array->data, array->capacity * sizeof(_TYPE)); \
	} \
	return array->size++; \
}
#endif

#pragma once
#include "MemoryPool.h"

//template<typename T>
//struct allocator
//{
//	typedef size_t    size_type;
//	typedef ptrdiff_t difference_type;
//	typedef T* pointer;
//	typedef const T* const_pointer;
//	typedef T& reference;
//	typedef const T& const_reference;
//	typedef T         value_type;
//
//	template <class U>
//	struct rebind { typedef allocator<U> other; };
//	enum
//	{
//		BASIC_NUM = 1000,
//	};
//	allocator()
//	{
//		MP = CreateMemoryPool(sizeof(T), BASIC_NUM);
//	}
//
//	~allocator()
//	{
//		int leakNum = ReportLeak(MP);
//		if (leakNum > 0) __debugbreak();
//		ReleaseMemoryPool(MP);
//	}
//
//	T* allocate(size_type n, const void* hint = 0)
//	{
//		if (n != 1)
//		{
//			__debugbreak();
//		}
//		return allocate(n);
//	}
//
//	pointer allocate(size_type n)
//	{
//		pointer pTemp = (pointer)AllocMemoryFromPool(MP);
//		new(pTemp)T{};
//		return pTemp;
//	}
//
//	void  deallocate(T* p, size_type n) {
//		if (n != 1)
//		{
//			__debugbreak();
//		}
//		RetMemoryToPool(MP, p);
//	}
//
//	MEMORYPOOL MP;
//};

#include "Allocator.h"

#include <crtdbg.h>

namespace Allocator
{
	[[nodiscard]] void* ProxyDebugHeap::malloc(std::size_t a_size)
	{
		return ::_malloc_dbg(a_size, _NORMAL_BLOCK, __FILE__, __LINE__);
	}

	[[nodiscard]] void* ProxyDebugHeap::aligned_alloc(std::size_t a_alignment, std::size_t a_size)
	{
		return ::_aligned_malloc_dbg(a_size, a_alignment, __FILE__, __LINE__);
	}

	[[nodiscard]] void* ProxyDebugHeap::realloc(void* a_ptr, std::size_t a_newSize)
	{
		return ::_realloc_dbg(a_ptr, a_newSize, _NORMAL_BLOCK, __FILE__, __LINE__);
	}

	[[nodiscard]] void* ProxyDebugHeap::aligned_realloc(std::size_t a_alignment, void* a_ptr, std::size_t a_newSize)
	{
		return ::_aligned_realloc_dbg(a_ptr, a_newSize, a_alignment, __FILE__, __LINE__);
	}

	void ProxyDebugHeap::free(void* a_ptr)
	{
		::_free_dbg(a_ptr, _NORMAL_BLOCK);
	}

	void ProxyDebugHeap::aligned_free(void* a_ptr)
	{
		::_aligned_free_dbg(a_ptr);
	}

	ProxyDebugHeap::ProxyDebugHeap() noexcept
	{
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
					   //_CRTDBG_DELAY_FREE_MEM_DF |
					   _CRTDBG_CHECK_ALWAYS_DF);
	}
}

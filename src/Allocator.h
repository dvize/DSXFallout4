#pragma once

namespace Allocator
{
	class ProxyHeap
	{
	public:
		ProxyHeap(const ProxyHeap&) = delete;
		ProxyHeap(ProxyHeap&&) = delete;
		ProxyHeap& operator=(const ProxyHeap&) = delete;
		ProxyHeap& operator=(ProxyHeap&&) = delete;

		[[nodiscard]] static ProxyHeap& get() noexcept
		{
			static ProxyHeap singleton;
			return singleton;
		}

		[[nodiscard]] void* malloc(std::size_t a_size) { return scalable_malloc(a_size); }

		[[nodiscard]] void* aligned_alloc(std::size_t a_alignment, std::size_t a_size) { return scalable_aligned_malloc(a_size, a_alignment); }

		[[nodiscard]] void* realloc(void* a_ptr, std::size_t a_newSize) { return scalable_realloc(a_ptr, a_newSize); }
		[[nodiscard]] void* aligned_realloc(std::size_t a_alignment, void* a_ptr, std::size_t a_newSize) { return scalable_aligned_realloc(a_ptr, a_newSize, a_alignment); }

		void free(void* a_ptr) { scalable_free(a_ptr); }
		void aligned_free(void* a_ptr) { scalable_aligned_free(a_ptr); }

	private:
		ProxyHeap() noexcept = default;
		~ProxyHeap() noexcept = default;
	};

	class ProxyDebugHeap
	{
	public:
		ProxyDebugHeap(const ProxyDebugHeap&) = delete;
		ProxyDebugHeap(ProxyDebugHeap&&) = delete;
		ProxyDebugHeap& operator=(const ProxyDebugHeap&) = delete;
		ProxyDebugHeap& operator=(ProxyDebugHeap&&) = delete;

		[[nodiscard]] static ProxyDebugHeap& get() noexcept
		{
			static ProxyDebugHeap singleton;
			return singleton;
		}

		[[nodiscard]] void* malloc(std::size_t a_size);
		[[nodiscard]] void* aligned_alloc(std::size_t a_alignment, std::size_t a_size);

		[[nodiscard]] void* realloc(void* a_ptr, std::size_t a_newSize);
		[[nodiscard]] void* aligned_realloc(std::size_t a_alignment, void* a_ptr, std::size_t a_newSize);

		void free(void* a_ptr);
		void aligned_free(void* a_ptr);

	private:
		ProxyDebugHeap() noexcept;
		~ProxyDebugHeap() noexcept = default;
	};

	class GameHeap
	{
	public:
		GameHeap() = default;
		GameHeap(const GameHeap&) = delete;
		GameHeap(GameHeap&&) = delete;
		~GameHeap() = default;
		GameHeap& operator=(const GameHeap&) = delete;
		GameHeap& operator=(GameHeap&&) = delete;

		[[nodiscard]] void* malloc(std::size_t a_size) { _allocate(_proxy, a_size, 0x0, false); }
		[[nodiscard]] void* aligned_alloc(std::size_t a_alignment, std::size_t a_size) { return _allocate(_proxy, a_size, static_cast<std::uint32_t>(a_alignment), true); }

		[[nodiscard]] void* realloc(void* a_ptr, std::size_t a_newSize) { return _reallocate(_proxy, a_ptr, a_newSize, 0x0, false); }
		[[nodiscard]] void* aligned_realloc(std::size_t a_alignment, void* a_ptr, std::size_t a_newSize) { return _reallocate(_proxy, a_ptr, a_newSize, static_cast<std::uint32_t>(a_alignment), true); }

		void free(void* a_ptr) { _deallocate(_proxy, a_ptr, false); }
		void aligned_free(void* a_ptr) { _deallocate(_proxy, a_ptr, true); }

	private:
		using Allocate_t = void*(RE::MemoryManager&, std::size_t, std::uint32_t, bool);
		using Deallocate_t = void(RE::MemoryManager&, void*, bool);
		using Reallocate_t = void*(RE::MemoryManager&, void*, std::size_t, std::uint32_t, bool);

		RE::MemoryManager& _proxy{ RE::MemoryManager::GetSingleton() };
		Allocate_t* const _allocate{ reinterpret_cast<Allocate_t*>(REL::ID(652767).address()) };
		Deallocate_t* const _deallocate{ reinterpret_cast<Deallocate_t*>(REL::ID(1582181).address()) };
		Reallocate_t* const _reallocate{ reinterpret_cast<Reallocate_t*>(REL::ID(1502917).address()) };
	};
}

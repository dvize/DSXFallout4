#include "hash.h"

#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <Windows.h>

#include <bcrypt.h>

#define NT_SUCCESS(a_status) (a_status >= 0)

namespace Hash
{
	std::optional<std::string> SHA512(std::span<const std::byte> a_data)
	{
		::BCRYPT_ALG_HANDLE algorithm;
		if (!NT_SUCCESS(::BCryptOpenAlgorithmProvider(
				&algorithm,
				BCRYPT_SHA512_ALGORITHM,
				nullptr,
				0))) {
			logger::error("failed to open algorithm provider"sv);
			return std::nullopt;
		}
		const stl::scope_exit delAlgorithm([&]() {
			[[maybe_unused]] const auto success = NT_SUCCESS(::BCryptCloseAlgorithmProvider(algorithm, 0));
			assert(success);
		});

		::BCRYPT_HASH_HANDLE hash;
		if (!NT_SUCCESS(::BCryptCreateHash(
				algorithm,
				&hash,
				nullptr,
				0,
				nullptr,
				0,
				0))) {
			logger::error("failed to create hash"sv);
			return std::nullopt;
		}
		const stl::scope_exit delHash([&]() {
			[[maybe_unused]] const auto success = NT_SUCCESS(::BCryptDestroyHash(hash));
			assert(success);
		});

		if (!NT_SUCCESS(::BCryptHashData(
				hash,
				reinterpret_cast<::PUCHAR>(const_cast<std::byte*>(a_data.data())),  // does not modify contents of buffer
				static_cast<::ULONG>(a_data.size()),
				0))) {
			logger::error("failed to hash data"sv);
			return std::nullopt;
		}

		::DWORD hashLen = 0;
		::ULONG discard = 0;
		if (!NT_SUCCESS(::BCryptGetProperty(
				hash,
				BCRYPT_HASH_LENGTH,
				reinterpret_cast<::PUCHAR>(&hashLen),
				sizeof(hashLen),
				&discard,
				0))) {
			logger::error("failed to get property"sv);
			return std::nullopt;
		}
		std::vector<::UCHAR> buffer(static_cast<std::size_t>(hashLen));

		if (!NT_SUCCESS(::BCryptFinishHash(
				hash,
				buffer.data(),
				static_cast<::ULONG>(buffer.size()),
				0))) {
			logger::error("failed to finish hash"sv);
			return std::nullopt;
		}

		std::string result;
		result.reserve(buffer.size() * 2);
		for (const auto byte : buffer) {
			result += fmt::format("{:02X}"sv, byte);
		}

		return { std::move(result) };
	}
}

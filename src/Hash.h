#pragma once

namespace Hash
{
	std::optional<std::string> SHA512(std::span<const std::byte> a_data);
}

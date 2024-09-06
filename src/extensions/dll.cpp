#include "extensions/dll.hpp"

fi::ext::dll::dll(const std::filesystem::path& dl_name)
    : dl_(dl_name.string())
{
}

const bool fi::ext::dll::valid() const
{
    return dl_.is_loaded();
}
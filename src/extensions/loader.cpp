#include "extensions/loader.hpp"

fi::ext::loader::loader(const std::filesystem::path& dl_name)
    : dl_(dl_name.string())
{
}

const bool fi::ext::loader::valid() const
{
    return dl_.is_loaded();
}
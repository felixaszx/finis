#include "extensions/loader.hpp"

using namespace fi;

ext_loader::ext_loader(const std::filesystem::path& dl_name)
    : dl_(dl_name.string())
{
}

const bool ext_loader::valid() const
{
    return dl_.is_loaded();
}


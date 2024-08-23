#include "extensions/loader.hpp"

using namespace fi;

ExtensionLoader::ExtensionLoader(const std::filesystem::path& dl_name)
    : dl_(dl_name.string())
{
}

const bool ExtensionLoader::valid() const
{
    return dl_.is_loaded();
}


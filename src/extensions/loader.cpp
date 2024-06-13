#include "extensions/loader.hpp"

using namespace fi;

ExtensionLoader::ExtensionLoader(const std::string& dl_name)
    : dl_(dl_name)
{
}

const bool ExtensionLoader::valid() const
{
    return dl_.is_loaded();
}

std::unique_ptr<Extension> ExtensionLoader::load_extension()
{
    std::unique_ptr<Extension> ext(dl_.get<Extension*()>("load_extension")());
    return ext;
}
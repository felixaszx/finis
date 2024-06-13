#ifndef INCLUDE_EXT_LOADER_HPP
#define INCLUDE_EXT_LOADER_HPP

#include <string>
#include <boost/dll.hpp>
#include "cpp_defines.hpp"

namespace fi
{ // the loading extension must be wirtten as a function
    class ExtensionLoader
    {
      private:
        boost::dll::shared_library dl_;

      public:
        ExtensionLoader(const std::string& dl_name);
        [[nodiscard]] const bool valid() const;
        [[nodiscard]] std::unique_ptr<Extension> load_extension();
    };
}; // namespace fi

#endif // INCLUDE_EXT_LOADER_HPP

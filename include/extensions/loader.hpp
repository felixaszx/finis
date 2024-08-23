#ifndef INCLUDE_EXT_LOADER_HPP
#define INCLUDE_EXT_LOADER_HPP

#include <string>
#include <filesystem>
#include <boost/dll.hpp>
#include "cpp_defines.hpp"
#include "tools.hpp"

namespace fi
{ // the loading extension must be wirtten as a function
    class ExtensionLoader
    {
      private:
        boost::dll::shared_library dl_;

      public:
        ExtensionLoader(const std::filesystem::path& dl_name);
        [[nodiscard]] const bool valid() const;

        template <typename T>
            requires std::derived_from<T, Extension>
        [[nodiscard]] std::unique_ptr<T> load_ext_unique(size_t idx = 0)
        {
            Extension* ext_ptr = dl_.get<Extension*()>(std::format("load_extension_{}", idx))();
            return std::unique_ptr<T>(dynamic_cast<T*>(ext_ptr));
        }

        template <typename T>
            requires std::derived_from<T, Extension>
        [[nodiscard]] std::shared_ptr<T> load_ext_shared(size_t idx = 0)
        {
            Extension* ext_ptr = dl_.get<Extension*()>(std::format("load_extension_{}", idx))();
            return std::shared_ptr<T>(dynamic_cast<T*>(ext_ptr));
        }
    };
}; // namespace fi

#endif // INCLUDE_EXT_LOADER_HPP
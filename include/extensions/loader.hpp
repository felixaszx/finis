#ifndef INCLUDE_EXT_LOADER_HPP
#define INCLUDE_EXT_LOADER_HPP

#include <string>
#include <filesystem>
#include <boost/dll.hpp>
#include "cpp_defines.hpp"
#include "tools.hpp"

namespace fi::ext
{ // the loading extension must be wirtten as a function
    class loader
    {
      private:
        boost::dll::shared_library dl_;

      public:
        loader(const std::filesystem::path& dl_name);
        [[nodiscard]] const bool valid() const;

        template <typename T>
            requires std::derived_from<T, base>
        [[nodiscard]] std::unique_ptr<T> load_unique(size_t idx = 0)
        {
            if (valid())
            {
                base* ext_ptr = dl_.get<base*()>(std::format("load_extension_{}", idx))();
                return std::unique_ptr<T>(util::castd<T*>(ext_ptr));
            }
            throw std::runtime_error(std::format("DLL {} is not loaded", dl_.location().generic_string()));
        }

        template <typename T>
            requires std::derived_from<T, base>
        [[nodiscard]] std::shared_ptr<T> load_shared(size_t idx = 0)
        {
            if (valid())
            {
                base* ext_ptr = dl_.get<base*()>(std::format("load_extension_{}", idx))();
                return std::shared_ptr<T>(util::castd<T*>(ext_ptr));
            }
            throw std::runtime_error(std::format("DLL {} is not loaded", dl_.location().generic_string()));
        }
    };
}; // namespace fi

#endif // INCLUDE_EXT_LOADER_HPP
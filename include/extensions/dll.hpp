#ifndef EXTENSIONS_DLL_HPP
#define EXTENSIONS_DLL_HPP

#include <string>
#include <filesystem>
#include <boost/dll.hpp>
#include "cpp_defines.hpp"
#include "tools.hpp"

namespace fi::ext
{ // the loading extension must be wirtten as a function
    class dll
    {
      private:
        boost::dll::shared_library dl_;

      public:
        dll(const std::filesystem::path& dl_name);
        [[nodiscard]] const bool valid() const;

        template <typename T>
        [[nodiscard]] std::unique_ptr<T> load_unique(size_t idx = 0)
        {
            if (valid())
            {
                void* ext_ptr = dl_.get<void*()>(std::format("load_extension_{}", idx))();
                return std::unique_ptr<T>((T*)(ext_ptr));
            }
            throw std::runtime_error(std::format("DLL {} is not loaded", dl_.location().generic_string()));
        }

        template <typename T>
        [[nodiscard]] std::shared_ptr<T> load_shared(size_t idx = 0)
        {
            if (valid())
            {
                void* ext_ptr = dl_.get<void*()>(std::format("load_extension_{}", idx))();
                return std::shared_ptr<T>((T*)(ext_ptr));
            }
            throw std::runtime_error(std::format("DLL {} is not loaded", dl_.location().generic_string()));
        }
    };
}; // namespace fi::ext

#endif // EXTENSIONS_DLL_HPP

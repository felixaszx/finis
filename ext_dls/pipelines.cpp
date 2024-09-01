#include <iostream>

#include "extensions/cpp_defines.hpp"
#include "tools.hpp"

class test_ext : public fi::ext::base
{
  private:
  public:
    test_ext() { std::cout << 1; }
};

EXPORT_EXTENSION(test_ext);
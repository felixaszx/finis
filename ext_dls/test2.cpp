#include <iostream>
#include "extensions/cpp_defines.hpp"

class Ext2 : public Extension
{
  private:
  public:
    Ext2() { usage_ = "testing"; }
};

EXPORT_EXTENSION(Ext2);
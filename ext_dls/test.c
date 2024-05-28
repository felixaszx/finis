
#include "extensions/defines.h"

const ObjectDetails* details = NULL;
PassChain* this_pass = NULL;

void init_(const ObjectDetails* details_in, PassChain* this_pass_in)
{
}

void clear()
{
}

void setup()
{
}

void render(VkCommandBuffer cmd)
{
}

void finish()
{
}

PassFunctions pass_func_getter()
{
    PassFunctions r;
    r.image_count_ = 4;
    r.init_ = init_;
    r.clear_ = clear;
    r.setup_ = setup;
    r.finish_ = finish;
    r.render_ = render;
    return r;
}
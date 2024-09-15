#include <stdio.h>
#include <stdlib.h>

#include "vk.h"

int main(int argc, char** argv)
{
    int* a = new (int);
    delete (a);
    return 0;
}
#include "src/nes.h"

#include <iostream>
#include <memory>

int main(int argc, char **argv)
{
    std::unique_ptr<NES> nes;

    if (argc != 2)
    {
        printf("[Ciel] Please provide one program argument!\n");
    }
    else
    {
        nes = std::make_unique<NES>(argv[1]);

        nes->run();
    }
}
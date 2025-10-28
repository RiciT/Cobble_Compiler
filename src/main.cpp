#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cerr << "Incorrect usage. Corrrect usage: " << std::endl;
        std::cerr << "cobble <input.cb>" << std::endl;
        return EXIT_FAILURE;
    }

    std::fstream input(argv[1], std::ios::in);
    return EXIT_SUCCESS;
} 
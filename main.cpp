#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Please, provide a test folder path as an argument." << std::endl;
    }

    const auto path = std::string(argv[1]) + "/test.txt";

    std::ifstream infile(path);
    if (!infile.is_open()) {
        std::cerr << "file " << path << " is not opened" << std::endl;
    }

    const int lines_number  = 15;
    const int symbols_number  = 15;

    int image[lines_number][symbols_number];

    for (int i = 0; i < lines_number; ++i) {
        std::string line;
        std::getline(infile, line);
        for (int j = 0; j < symbols_number; ++j) {
            image[i][j] = line.at(j) - '0';
        }
    }

    infile.close();

    for (auto &row : image) {
        for (int symb : row) {
            std::cout << symb;
        }
        std::cout << std::endl;
    }

    return 0;
}
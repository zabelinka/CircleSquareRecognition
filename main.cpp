#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <opencv2/imgproc.hpp>
#include <opencv/highgui.h>

using namespace std;
using namespace cv;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Please, provide a path to the test file as an argument." << std::endl;
        return 1;
    }

    const auto filepath = std::string(argv[1]);

    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cerr << "File " << filepath << " can not be opened" << std::endl;
        return 1;
    }

    const int lines_number  = 15;
    const int symbols_number  = 15;

    cv::Mat binary(lines_number, symbols_number, CV_8UC1);

    for (int i = 0; i < lines_number; ++i) {
        std::string line;
        std::getline(infile, line);
        for (int j = 0; j < symbols_number; ++j) {
            binary.at<uchar>(i, j) = uchar(line.at(j) - '0');
        }
    }

    std::cout << "Origin = " << std::endl << binary << std::endl;



    uchar kernel_data[9] = {0, 1, 0, 1, 1, 1, 0, 1, 0};
    cv::Mat kernel = cv::Mat(3, 3, CV_8UC1, kernel_data);
    std::cout << "Kernel = " << std::endl << kernel << std::endl;

    Mat opened = Mat::zeros(binary.size(), CV_8UC1);
    morphologyEx(binary, opened, MORPH_OPEN, kernel);
    std::cout << "Opened = " << std::endl << opened << std::endl;

    cv::Mat diff = binary - opened;
    std::cout << "Diff = " << std::endl << diff << std::endl;



    return 0;
}
#include <iostream>
#include <fstream>
#include <string>

#include <opencv2/imgproc.hpp>
#include <opencv/highgui.h>

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

    std::cout << "Source image = " << std::endl << binary << std::endl;

    cv::Mat labeled, statistics, centroid;
    const int number_of_labels = cv::connectedComponentsWithStats(binary, labeled, statistics, centroid, 8);

    cv::Rect biggest_bb;

    // skip background label 0
    for (int i = 1; i < number_of_labels; ++i) {
        const cv::Size size(statistics.at<int>(i, cv::CC_STAT_WIDTH), statistics.at<int>(i, cv::CC_STAT_HEIGHT));

        if (size.width > biggest_bb.width || size.height > biggest_bb.height) {
            const cv::Point corner(statistics.at<int>(i, cv::CC_STAT_LEFT), statistics.at<int>(i,cv::CC_STAT_TOP));
            biggest_bb = cv::Rect(corner, size);
        }
    }

    std::cout << "Found biggest bounding box: " << biggest_bb << std::endl;

    if (biggest_bb.width != biggest_bb.height) {
        std::cout << "Unknown shape. Bounding box has different width and height." << std::endl;
        return 0;
    }
    if (biggest_bb.width < 5) {
        std::cout << "Unknown shape. Width is too small to be valid. Actual: "
                  << biggest_bb.width << ", expected greater than 5." << std::endl;
        return 0;
    }
    if (biggest_bb.width > 10) {
        std::cout << "Unknown shape. Width is too big to be valid. Actual: "
                  << biggest_bb.width << ", expected less than 10." << std::endl;
        return 0;
    }

    uchar kernel_data[9] = {0, 1, 0, 1, 1, 1, 0, 1, 0};
    const cv::Mat kernel = cv::Mat(3, 3, CV_8UC1, kernel_data);

    cv::Mat opened = cv::Mat::zeros(binary.size(), CV_8UC1);
    morphologyEx(binary, opened, cv::MORPH_OPEN, kernel);
    std::cout << "Opened image = " << std::endl << opened << std::endl;

    cv::Mat binary_bb = binary(biggest_bb);
    cv::Mat opened_bb = opened(biggest_bb);

    const cv::Mat diff = binary_bb - opened_bb;
    std::cout << "Diff image = " << std::endl << diff << std::endl;

    std::vector<cv::Point> diff_points;
    for (int y = 0; y < diff.rows; ++y) {
        for (int x = 0; x < diff.cols; ++x) {
            if (diff.at<uchar>(y, x) == 1) {
                diff_points.emplace_back(x, y);
            }
        }
    }

    if (diff_points.empty()) {
        std::cout << "Circle. Confidence: 1.0" << std::endl;
        const cv::Point center = (biggest_bb.br() + biggest_bb.tl()) * 0.5;
        std::cout << "Diameter: " << biggest_bb.width << ". Center: " << center << std::endl;
        return 0;
    }

    double confidence = 0.0;
    if (diff.at<uchar>(0, 0) == 1) {
        confidence += 0.25;
    }
    if (diff.at<uchar>(0, diff.cols - 1) == 1) {
        confidence += 0.25;
    }
    if (diff.at<uchar>(diff.rows - 1, 0) == 1) {
        confidence += 0.25;
    }
    if (diff.at<uchar>(diff.rows - 1, diff.cols - 1) == 1) {
        confidence += 0.25;
    }

    std::cout << "Square. Confidence: " << confidence << std::endl;
    std::cout << "Side length: " << biggest_bb.width << ". Top left corner: " << biggest_bb.tl() << std::endl;



    return 0;
}
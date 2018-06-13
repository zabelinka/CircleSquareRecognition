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

    cv::Mat source(lines_number, symbols_number, CV_8UC1);

    for (int i = 0; i < lines_number; ++i) {
        std::string line;
        std::getline(infile, line);
        for (int j = 0; j < symbols_number; ++j) {
            source.at<uchar>(i, j) = uchar(line.at(j) - '0');
        }
    }

    cv::Mat labeled, statistics, centroid;
    const int number_of_labels = cv::connectedComponentsWithStats(source, labeled, statistics, centroid, 8);

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

    double confidence = 1.0;
    const double decreasing_factor = 0.9;
    const double hard_decreasing_factor = 0.8;

    if (biggest_bb.width != biggest_bb.height) {
        std::cout << "Bounding box has different width and height." << std::endl;
        confidence *= hard_decreasing_factor;
    }
    if (biggest_bb.width < 5) {
        std::cout << "Width is too small to be valid. Actual: "
                  << biggest_bb.width << ", expected in range [5, 10]." << std::endl;
        confidence *= hard_decreasing_factor;
    }
    if (biggest_bb.width > 10) {
        std::cout << "Width is too big to be valid. Actual: "
                  << biggest_bb.width << ", expected in range [5, 10]." << std::endl;
        confidence *= hard_decreasing_factor;
    }

    const cv::Mat source_bb = source(biggest_bb);
    std::cout << "Source image bounding box: " << std::endl << source_bb << std::endl;

    uchar kernel_data[9] = {0, 1, 0, 1, 1, 1, 0, 1, 0};
    const cv::Mat kernel = cv::Mat(3, 3, CV_8UC1, kernel_data);

    cv::Mat opened_bb = cv::Mat::zeros(source_bb.size(), CV_8UC1);
    morphologyEx(source_bb, opened_bb, cv::MORPH_OPEN, kernel);
    std::cout << "Opened image: " << std::endl << opened_bb << std::endl;

    const cv::Mat diff = source_bb - opened_bb;
    std::cout << "Diff image: " << std::endl << diff << std::endl;

    std::vector<cv::Point> diff_points;
    for (int y = 0; y < diff.rows; ++y) {
        for (int x = 0; x < diff.cols; ++x) {
            if (diff.at<uchar>(y, x) == 1) {
                diff_points.emplace_back(x, y);
            }
        }
    }

    if (diff_points.empty()) {
        const cv::Point center = (biggest_bb.br() + biggest_bb.tl()) * 0.5;
        std::cout << "Circle. Confidence: " << confidence
                  << ". Diameter: " << biggest_bb.width
                  << ". Center: " << center << std::endl;
        return 0;
    }

    int missing_corners = 0;
    if (diff.at<uchar>(0, 0) != 1) {
        ++missing_corners;
    }
    if (diff.at<uchar>(0, diff.cols - 1) != 1) {
        ++missing_corners;
    }
    if (diff.at<uchar>(diff.rows - 1, 0) != 1) {
        ++missing_corners;
    }
    if (diff.at<uchar>(diff.rows - 1, diff.cols - 1) != 1) {
        ++missing_corners;
    }

    confidence *= std::pow(decreasing_factor, missing_corners);

    if (confidence < 0.5 || missing_corners == 4) {
        std::cout << "Unknown shape. Confidence: " << confidence << std::endl;
        return 0;
    }

    std::cout << "Square. Confidence: " << confidence
              << ". Side length: " << biggest_bb.width
              << ". Top left corner: " << biggest_bb.tl() << std::endl;

    return 0;
}
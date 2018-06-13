// include: STL
#include <iostream>
#include <fstream>
#include <string>

// include: OpenCV
#include <opencv2/imgproc.hpp>
#include <opencv/highgui.h>

const int lines_number  = 15;
const int symbols_number  = 15;

const int min_diameter  = 5;
const int max_diameter  = 10;

const double decreasing_factor = 0.9;
const double hard_decreasing_factor = 0.8;

cv::Mat read_image_from_file(const std::string& file_path) {
    std::ifstream infile(file_path);
    if (!infile.is_open()) {
        std::cerr << "File " << file_path << " can not be opened" << std::endl;
        return cv::Mat();
    }

    cv::Mat source(lines_number, symbols_number, CV_8UC1);

    for (int i = 0; i < lines_number; ++i) {
        std::string line;
        std::getline(infile, line);
        for (int j = 0; j < symbols_number; ++j) {
            source.at<uchar>(i, j) = uchar(line.at(j) - '0');
        }
    }

    return source;
}

cv::Rect find_biggest_component_bb(const cv::Mat &source) {
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
    return biggest_bb;
}

void decrease_confidence_for_bad_bb(double &confidence, const cv::Rect &bb) {

    if (bb.width != bb.height) {
        std::cout << "Bounding box has different width and height." << std::endl;
        confidence *= hard_decreasing_factor;
    }
    if (bb.width < min_diameter) {
        std::cout << "Width is too small to be valid. Actual: " << bb.width
                  << ", expected in range [" << min_diameter << ", " << max_diameter <<"]." << std::endl;
        confidence *= hard_decreasing_factor;
    }
    if (bb.width > max_diameter) {
        std::cout << "Width is too big to be valid. Actual: "<< bb.width
                << ", expected in range [" << min_diameter << ", " << max_diameter <<"]." << std::endl;
        confidence *= hard_decreasing_factor;
    }
}

std::vector<cv::Point> get_black_points(const cv::Mat &img) {
    std::vector<cv::Point> points;
    for (int y = 0; y < img.rows; ++y) {
        for (int x = 0; x < img.cols; ++x) {
            if (img.at<uchar>(y, x) == 1) {
                points.emplace_back(x, y);
            }
        }
    }
    return points;
}

int count_missing_corners(const cv::Mat &img){
    int missing_corners = 0;
    if (img.at<uchar>(0, 0) != 1) {
        ++missing_corners;
    }
    if (img.at<uchar>(0, img.cols - 1) != 1) {
        ++missing_corners;
    }
    if (img.at<uchar>(img.rows - 1, 0) != 1) {
        ++missing_corners;
    }
    if (img.at<uchar>(img.rows - 1, img.cols - 1) != 1) {
        ++missing_corners;
    }
    return missing_corners;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Please, provide a path to the test file as an argument." << std::endl;
        return 1;
    }

    const auto file_path = std::string(argv[1]);

    const cv::Mat source = read_image_from_file(file_path);
    if (source.empty()) {
        std::cerr << "Cannot read image" << std::endl;
        return 1;
    }

    const auto biggest_bb = find_biggest_component_bb(source);
    std::cout << "Found biggest bounding box: " << biggest_bb << std::endl;

    double confidence = 1.0;
    decrease_confidence_for_bad_bb(confidence, biggest_bb);

    const cv::Mat source_bb = source(biggest_bb);
    std::cout << "Source image bounding box: " << std::endl << source_bb << std::endl;

    uchar kernel_data[9] = {0, 1, 0, 1, 1, 1, 0, 1, 0};
    const cv::Mat kernel = cv::Mat(3, 3, CV_8UC1, kernel_data);

    cv::Mat opened_bb = cv::Mat::zeros(source_bb.size(), CV_8UC1);
    morphologyEx(source_bb, opened_bb, cv::MORPH_OPEN, kernel);
    std::cout << "Opened image: " << std::endl << opened_bb << std::endl;

    const cv::Mat diff = source_bb - opened_bb;
    std::cout << "Diff image: " << std::endl << diff << std::endl;

    const auto diff_points = get_black_points(diff);

    if (diff_points.empty()) {
        const cv::Point center = (biggest_bb.br() + biggest_bb.tl()) * 0.5;
        std::cout << "Circle. Confidence: " << confidence
                  << ". Diameter: " << biggest_bb.width
                  << ". Center: " << center << std::endl;
        return 0;
    }

    const int missing_corners = count_missing_corners(diff);
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
#include "native_sudoku_scanner.hpp"
#include "detection/grid_detector.hpp"
#include "dictionary/dictionary.hpp"
#include "extraction/grid_extractor.hpp"
#include "extraction/structs/cell.hpp"
#include <opencv2/opencv.hpp>

extern "C" __attribute__((visibility("default"))) __attribute__((used)) struct Coordinate *create_coordinate(double x, double y) {
    struct Coordinate *coordinate = (struct Coordinate *)malloc(sizeof(struct Coordinate));
    coordinate->x = x;
    coordinate->y = y;
    return coordinate;
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) struct DetectionResult *create_detection_result(Coordinate *topLeft, Coordinate *topRight, Coordinate *bottomLeft, Coordinate *bottomRight) {
    struct DetectionResult *detectionResult = (struct DetectionResult *)malloc(sizeof(struct DetectionResult));
    detectionResult->topLeft = topLeft;
    detectionResult->topRight = topRight;
    detectionResult->bottomLeft = bottomLeft;
    detectionResult->bottomRight = bottomRight;
    return detectionResult;
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) struct DetectionResult *detect_grid(char *path, double roiSize, double roiOffset, double aspectRatio) {
    // struct DetectionResult *coordinate = (struct DetectionResult *)malloc(sizeof(struct DetectionResult));
    cv::Mat mat = cv::imread(path);

    if (mat.size().width == 0 || mat.size().height == 0) {
        return create_detection_result(
            create_coordinate(0, 0),
            create_coordinate(1, 0),
            create_coordinate(0, 1),
            create_coordinate(1, 1));
    }

    // crop image so it only contains ROI
    if (roiSize > 0 && roiOffset > 0 && aspectRatio > 0) {

        // fit height or width depending on given aspect ratio
        bool fit_height = (aspectRatio > mat.size().height / mat.size().width);

        // first get resulting dimensions for given aspect ratio
        double height = fit_height ? mat.size().height : mat.size().width * aspectRatio;
        double width = fit_height ? mat.size().height / aspectRatio : mat.size().width;

        // define size and place of ROI in pixels
        double roi_size = roiSize * width; // roiSize is given as a percentage
        double roi_offset = roiOffset * height; // roiOffset is given as a percentage

        // calculate start and end points of ROI and stay in bounderies of image
        int x_start = cv::max((mat.size().width / 2) - (roi_size / 2), 0.0);
        int x_end = cv::min(x_start + roi_size, mat.size().width - 1.0);
        int y_start = cv::max((mat.size().height / 2) - (roi_size / 2) - roi_offset, 0.0);
        int y_end = cv::min(y_start + roi_size, mat.size().height - 1.0);

        mat = mat(cv::Range(y_start, y_end), cv::Range(x_start, x_end));
        cv::imwrite(path, mat);
    }

    std::vector<cv::Point> points = GridDetector::detect_grid(mat);

    return create_detection_result(
        create_coordinate((double)points[0].x / mat.size().width, (double)points[0].y / mat.size().height),
        create_coordinate((double)points[1].x / mat.size().width, (double)points[1].y / mat.size().height),
        create_coordinate((double)points[2].x / mat.size().width, (double)points[2].y / mat.size().height),
        create_coordinate((double)points[3].x / mat.size().width, (double)points[3].y / mat.size().height));
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) int *extract_grid(
    char *path,
    double topLeftX,
    double topLeftY,
    double topRightX,
    double topRightY,
    double bottomLeftX,
    double bottomLeftY,
    double bottomRightX,
    double bottomRightY) {
    cv::Mat mat = cv::imread(path);

    std::vector<int> grid = GridExtractor::extract_grid(
        mat,
        topLeftX * mat.size().width,
        topLeftY * mat.size().height,
        topRightX * mat.size().width,
        topRightY * mat.size().height,
        bottomLeftX * mat.size().width,
        bottomLeftY * mat.size().height,
        bottomRightX * mat.size().width,
        bottomRightY * mat.size().height);

    int *grid_ptr = (int*)malloc(grid.size() * sizeof(int));

    // copy grid_array to pointer
    for (int i = 0; i < grid.size(); ++i) {
        grid_ptr[i] = grid[i];
    }

    return grid_ptr;
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) bool debug_grid_detection(char *path) {
    cv::Mat thresholded;
    cv::Mat img = cv::imread(path);

    cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
    // always check parameters with grid_detector.cpp
    cv::adaptiveThreshold(img, thresholded, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 69, 20);

    return cv::imwrite(path, thresholded);
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) bool debug_grid_extraction(
    char *path,
    double topLeftX,
    double topLeftY,
    double topRightX,
    double topRightY,
    double bottomLeftX,
    double bottomLeftY,
    double bottomRightX,
    double bottomRightY) {
    cv::Mat transformed;
    cv::Mat thresholded;
    cv::Mat img = cv::imread(path);

    cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
    transformed = GridExtractor::crop_and_transform(
        img,
        topLeftX * img.size().width,
        topLeftY * img.size().height,
        topRightX * img.size().width,
        topRightY * img.size().height,
        bottomLeftX * img.size().width,
        bottomLeftY * img.size().height,
        bottomRightX * img.size().width,
        bottomRightY * img.size().height);
    // always check parameters with grid_extractor.cpp
    cv::adaptiveThreshold(transformed, thresholded, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 63, 10);

    std::vector<Cell> cells = GridExtractor::extract_cells(thresholded, transformed);
    cv::Mat stitched = GridExtractor::stitch_cells(cells);

    return cv::imwrite(path, stitched);
}

extern "C" __attribute__((visibility("default"))) __attribute__((used)) void set_model(char *path) {
    setenv(PATH_TO_MODEL_ENV_VAR, path, 1);
}
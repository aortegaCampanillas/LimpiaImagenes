#pragma once

#include <filesystem>

#include <opencv2/core.hpp>

struct Config {
    int strength = 2;
    int threshold = 130;
    int delta = 8;
    int expand = 4;
    int mask_size = 9;
    int fill_size = 6;
};

int normalize_filter_size(int value, int minimum);
void load_config(const std::filesystem::path& path, Config& config);
cv::Mat denoise_image(const cv::Mat& image, const Config& config);

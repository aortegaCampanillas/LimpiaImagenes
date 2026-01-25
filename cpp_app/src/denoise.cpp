#include "denoise.h"

#include <charconv>
#include <fstream>
#include <optional>
#include <string>

#include <opencv2/imgproc.hpp>

namespace {
std::optional<int> parse_int_field(const std::string& text, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    std::size_t pos = text.find(needle);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    pos = text.find(':', pos);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    pos += 1;
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        pos += 1;
    }
    const char* begin = text.data() + pos;
    const char* end = text.data() + text.size();
    int value = 0;
    auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc()) {
        return std::nullopt;
    }
    return value;
}
}  // namespace

int normalize_filter_size(int value, int minimum) {
    if (value < minimum) {
        return minimum;
    }
    if (value % 2 == 0) {
        return value + 1;
    }
    return value;
}

void load_config(const std::filesystem::path& path, Config& config) {
    std::ifstream input(path);
    if (!input) {
        return;
    }
    std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (auto value = parse_int_field(content, "strength")) {
        config.strength = *value;
    }
    if (auto value = parse_int_field(content, "threshold")) {
        config.threshold = *value;
    }
    if (auto value = parse_int_field(content, "delta")) {
        config.delta = *value;
    }
    if (auto value = parse_int_field(content, "expand")) {
        config.expand = *value;
    }
    if (auto value = parse_int_field(content, "mask_size")) {
        config.mask_size = *value;
    }
    if (auto value = parse_int_field(content, "fill_size")) {
        config.fill_size = *value;
    }
}

cv::Mat denoise_image(const cv::Mat& image, const Config& config) {
    int fill_size = normalize_filter_size(config.fill_size, 3);
    int mask_size = normalize_filter_size(config.mask_size, 3);
    int expand = normalize_filter_size(config.expand, 1);
    int strength = std::max(1, config.strength);

    cv::Mat filled = image.clone();
    for (int i = 0; i < strength; ++i) {
        cv::medianBlur(filled, filled, fill_size);
    }

    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    cv::Mat local_median;
    cv::medianBlur(gray, local_median, mask_size);

    cv::Mat dark_mask;
    cv::compare(gray, config.threshold, dark_mask, cv::CMP_LE);

    cv::Mat diff;
    cv::subtract(local_median, gray, diff);

    cv::Mat diff_mask;
    cv::compare(diff, config.delta, diff_mask, cv::CMP_GE);

    cv::Mat mask;
    cv::bitwise_and(dark_mask, diff_mask, mask);

    if (expand > 1) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(expand, expand));
        cv::dilate(mask, mask, kernel);
    }

    cv::Mat cleaned = image.clone();
    filled.copyTo(cleaned, mask);
    return cleaned;
}

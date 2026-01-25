#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include <opencv2/imgcodecs.hpp>

#include "denoise.h"

void print_usage(const char* exe) {
    std::cout << "Uso:\n"
              << "  " << exe << " --input <ruta> --output <ruta> [opciones]\n\n"
              << "Opciones:\n"
              << "  --config <ruta>     Ruta a config.json (opcional)\n"
              << "  --strength <n>      Intensidad (1-5)\n"
              << "  --threshold <n>     Umbral (10-160)\n"
              << "  --delta <n>         Contraste (2-40)\n"
              << "  --mask-size <n>     Mascara (3-15)\n"
              << "  --fill-size <n>     Relleno (3-11)\n"
              << "  --expand <n>        Expansion (1-7)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        print_usage(argv[0]);
        return 1;
    }

    std::filesystem::path input_path;
    std::filesystem::path output_path;
    std::optional<std::filesystem::path> config_path;
    Config config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) {
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--input") {
            if (const char* value = next()) {
                input_path = value;
            }
        } else if (arg == "--output") {
            if (const char* value = next()) {
                output_path = value;
            }
        } else if (arg == "--config") {
            if (const char* value = next()) {
                config_path = value;
            }
        } else if (arg == "--strength") {
            if (const char* value = next()) {
                config.strength = std::stoi(value);
            }
        } else if (arg == "--threshold") {
            if (const char* value = next()) {
                config.threshold = std::stoi(value);
            }
        } else if (arg == "--delta") {
            if (const char* value = next()) {
                config.delta = std::stoi(value);
            }
        } else if (arg == "--mask-size") {
            if (const char* value = next()) {
                config.mask_size = std::stoi(value);
            }
        } else if (arg == "--fill-size") {
            if (const char* value = next()) {
                config.fill_size = std::stoi(value);
            }
        } else if (arg == "--expand") {
            if (const char* value = next()) {
                config.expand = std::stoi(value);
            }
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (input_path.empty() || output_path.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    if (config_path) {
        load_config(*config_path, config);
    } else {
        std::filesystem::path default_path = std::filesystem::path("..") / "config.json";
        load_config(default_path, config);
    }

    cv::Mat image = cv::imread(input_path.string(), cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "No se pudo abrir la imagen: " << input_path << "\n";
        return 1;
    }

    cv::Mat cleaned = denoise_image(image, config);
    if (!cv::imwrite(output_path.string(), cleaned)) {
        std::cerr << "No se pudo guardar la imagen: " << output_path << "\n";
        return 1;
    }

    std::cout << "Imagen procesada: " << output_path << "\n";
    return 0;
}

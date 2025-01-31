#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <opencv2/opencv.hpp>
#include <chrono>

namespace fs = std::filesystem;

void convert_image(const fs::path& input_path, const fs::path& output_path) {
    cv::Mat image = cv::imread(input_path.string(), cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        std::cerr << "無法讀取圖片: " << input_path << std::endl;
        return;
    }
    cv::imwrite(output_path.string(), image);
}

void process_chunk(const std::vector<fs::path>& chunk, const fs::path& output_dir) {
    for (const auto& file : chunk) {
        fs::path output_path = output_dir / file.filename().replace_extension(".png");
        convert_image(file, output_path);
    }
}

std::vector<fs::path> get_bmp_files(const fs::path& path) {
    std::vector<fs::path> bmp_files;
    if (fs::is_regular_file(path) && path.extension() == ".bmp") {
        bmp_files.push_back(path);
    } else if (fs::is_directory(path)) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (fs::is_regular_file(entry) && entry.path().extension() == ".bmp") {
                bmp_files.push_back(entry.path());
            }
        }
    }
    return bmp_files;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "使用方法: " << argv[0] << " <輸入路徑> <進程數>" << std::endl;
        return 1;
    }

    fs::path input_path = argv[1];
    int num_processes = std::stoi(argv[2]);

    std::vector<fs::path> bmp_files = get_bmp_files(input_path);

    // 創建輸出目錄
    fs::path output_dir = fs::current_path() / "converted_multi";
    fs::create_directories(output_dir);

    size_t chunk_size = (bmp_files.size() + num_processes - 1) / num_processes;

    auto start = std::chrono::high_resolution_clock::now(); // 開始計時
    for (int i = 0; i < num_processes; ++i) {
        pid_t pid = fork();

        if (pid == 0) {  // 子進程
            auto start = bmp_files.begin() + i * chunk_size;
            auto end = (i == num_processes - 1) ? bmp_files.end() : start + chunk_size;
            std::vector<fs::path> chunk(start, end);

            process_chunk(chunk, output_dir);
            exit(0);
        } else if (pid < 0) {
            std::cerr << "Fork 失敗" << std::endl;
            return 1;
        }
    }

    // 等待所有子進程結束
    for (int i = 0; i < num_processes; ++i) {
        wait(NULL);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "排序完成，耗時: " << diff.count() << " 秒" << std::endl;

    std::cout << "轉換完成！檔案已保存在 " << output_dir << std::endl;

    return 0;
}
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;

std::uint64_t hash_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + path.string());

    std::uint64_t hash = 14695981039346656037ULL;
    std::array<char, 16384> buffer{};

    while (true) {
        file.read(buffer.data(), buffer.size());
        const auto count = file.gcount();

        for (std::streamsize i = 0; i < count; ++i) {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(i)]);
            hash *= 1099511628211ULL;
        }

        if (count < static_cast<std::streamsize>(buffer.size())) {
            if (!file.eof()) throw std::runtime_error("Cannot read file: " + path.string());
            return hash;
        }
    }
}

bool same_contents(const fs::path& first_path, const fs::path& second_path) {
    std::ifstream first(first_path, std::ios::binary);
    std::ifstream second(second_path, std::ios::binary);
    if (!first || !second) throw std::runtime_error("Cannot open files for comparison");

    std::array<char, 16384> first_buffer{};
    std::array<char, 16384> second_buffer{};

    while (true) {
        first.read(first_buffer.data(), first_buffer.size());
        second.read(second_buffer.data(), second_buffer.size());
        const auto first_count = first.gcount();
        const auto second_count = second.gcount();

        if ((first.fail() && !first.eof()) || (second.fail() && !second.eof())) {
            throw std::runtime_error("Cannot read files for comparison");
        }
        if (first_count != second_count ||
            !std::equal(first_buffer.data(), first_buffer.data() + first_count,
                        second_buffer.data())) {
            return false;
        }
        if (first_count < static_cast<std::streamsize>(first_buffer.size())) return true;
    }
}

void replace_with_link(const fs::path& original, const fs::path& duplicate) {
    const fs::path temporary = duplicate.parent_path() /
                               (".hardlink-tmp-" + std::to_string(getpid()));
    fs::create_hard_link(original, temporary);

    try {
        fs::rename(temporary, duplicate);
    } catch (...) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " DIRECTORY\n";
        return 1;
    }

    try {
        const fs::path root = fs::absolute(argv[1]);
        if (!fs::is_directory(root)) {
            std::cerr << "Not a directory: " << root << '\n';
            return 1;
        }

        std::vector<fs::path> files;
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (fs::is_regular_file(entry.symlink_status())) files.push_back(entry.path());
        }

        using Key = std::pair<std::uintmax_t, std::uint64_t>;
        std::map<Key, std::vector<fs::path>> groups;
        for (const auto& path : files) {
            groups[{fs::file_size(path), hash_file(path)}].push_back(path);
        }

        std::size_t replaced = 0;
        for (const auto& group : groups) {
            std::vector<fs::path> originals;

            for (const auto& path : group.second) {
                bool found = false;
                for (const auto& original : originals) {
                    if (!same_contents(original, path)) continue;

                    if (!fs::equivalent(original, path)) {
                        replace_with_link(original, path);
                        std::cout << path << " -> " << original << '\n';
                        ++replaced;
                    }
                    found = true;
                    break;
                }
                if (!found) originals.push_back(path);
            }
        }

        std::cout << "Files checked: " << files.size() << '\n';
        std::cout << "Files replaced: " << replaced << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 2;
    }

    return 0;
}

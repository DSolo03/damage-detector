#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <cstdlib>
#include <sstream>
#include <filesystem>
#include <set>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <chrono>
#include <random>

#include "shortcut.h"

namespace fs = std::filesystem;

std::vector<std::string> video_extensions = {"mp4","3gp","asf","avi","dav","divx","vob","f4v","flv","m4v","mkv","m4p","m4v","mpeg","mpg","mp2","mpe","mpv","m2v","mts","ts","ogg","mov","qt","rm","rmvb","webm","wmv"};

struct ZeroChunk {
    uint64_t offset;
    uint64_t length;
};

std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

bool hasExtension(const std::string& filename, const std::vector<std::string>& extensions) {
    auto pos = filename.find_last_of('.');
    if (pos == std::string::npos || pos == filename.size() - 1) {
        return false;
    }
    std::string ext = filename.substr(pos + 1);

    ext = toLower(filename.substr(pos + 1));
    
    for (const auto& e : extensions) {
        if (ext == e) {
            return true;
        }
    }
    return false;
}

std::string randomAlnum(size_t length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(gen)];
    }
    return result;
}

std::vector<ZeroChunk> findZeroChunks(const fs::path& filepath, uint64_t& min_size, uint64_t& file_size, std::size_t buf_size = 16 * 1024 * 1024,uint64_t ignore_treshold=0) {
    std::vector<ZeroChunk> chunks;

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath.u8string());
    }
    std::streamsize size = file.tellg();
    file_size=size;
    file.seekg(0, std::ios::beg);
    
    if (!hasExtension(filepath.u8string(),video_extensions)){
        return chunks;
    }

    uint64_t start_offset = ignore_treshold;
    uint64_t end_offset = (ignore_treshold > size) ? 0 : size - ignore_treshold;

    if (start_offset >= end_offset){
        start_offset=0;
        end_offset=size;
    }

    file.seekg(static_cast<std::streamoff>(start_offset));

    std::vector<unsigned char> buffer(buf_size);
    uint64_t file_offset = start_offset;
    bool in_zero = false;
    uint64_t zero_start = 0;

    uint64_t current_offset = start_offset;

    if (min_size==1){
        min_size=static_cast<uint64_t>(size*5/100);
    }

    while (current_offset < end_offset) {
        file_offset = current_offset;
        std::size_t bytes_to_read = buf_size;
        if (current_offset + bytes_to_read > end_offset)
            bytes_to_read = static_cast<std::size_t>(end_offset - current_offset);
        file.read(reinterpret_cast<char*>(buffer.data()), bytes_to_read);
        std::streamsize bytes_read = file.gcount();

        if (bytes_read <= 0) break;
        
        for (std::streamsize i = 0; i < bytes_read; ++i) {
            if (buffer[static_cast<size_t>(i)] == 0u) {
                if (!in_zero) {
                    in_zero = true;
                    zero_start = file_offset + static_cast<uint64_t>(i);
                }
            } else if (in_zero) {
                uint64_t length = (file_offset + static_cast<uint64_t>(i)) - zero_start;
                if (length >= min_size) chunks.push_back({zero_start, length});
                in_zero = false;
            }
        }
        current_offset += bytes_read;
    }

    if (in_zero) {
        uint64_t length = end_offset - zero_start;
        if (length >= min_size) chunks.push_back({zero_start, length});
    }

    return chunks;
}

std::string humanSize(unsigned long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int i = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && i < 5) {
        size /= 1024.0;
        i++;
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision((size < 10 && i > 0) ? 2 : 0) << size << " " << units[i];
    return out.str();
}

static void printChunk(const ZeroChunk& c) {
    std::cout << "Zero Chunk. Offset: " << humanSize(c.offset)
              << " (0x" << std::hex << c.offset << std::dec << ")"
              << " Length: " << humanSize(c.length)
              << " (0x" << std::hex << c.length << std::dec << ")\n";
}

void processFile(const fs::path& path, uint64_t min_size, std::set<fs::path>& files_with_zeros,uint64_t buffer_size,uint64_t ignore_treshold=0) {
    try {
        uint64_t min_size_temp=min_size;
        uint64_t file_size=0;
        auto chunks = findZeroChunks(path, min_size_temp,file_size,buffer_size,ignore_treshold); 
        //std::cout << "File: " << path.u8string() << ", min_size=" << min_size_temp << "\n"; 
        if (!chunks.empty()) {
            std::cout << "File: " << path.u8string() << " Size: " << humanSize(file_size) << "\n";
            for (const auto& c : chunks) printChunk(c);
            std::cout << "\n";
            files_with_zeros.insert(path);
            std::wstring filename = to_wchar(randomAlnum(4)+" "+path.stem().u8string());
            std::filesystem::path shortcutFolder = "Shortcuts";
            std::filesystem::path shortcutPath = shortcutFolder / (filename + L".lnk");
            std::wstring target = to_wchar(path.u8string());
            CreateShortcut(target.c_str(), shortcutPath.c_str(), filename.c_str());
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing file: " << path.u8string() << " (" << e.what() << ")\n";
    }
}

size_t countFilesInDirectory(const fs::path& dir) {
    size_t count = 0;
    for (auto& entry : fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied)) {
        try {
            if (fs::is_regular_file(entry.path())) count++;
        } catch (...) {
            continue;
        }
    }
    return count;
}

void processPath(const fs::path& path, uint64_t min_size, std::set<fs::path>& files_with_zeros,uint64_t buffer_size,uint64_t ignore_treshold=0) {
    try {
        if (!fs::exists(path)) {
            std::cerr << "Path does not exist: " << path.u8string() << "\n";
            return;
        }
        size_t file_current_count=0;
        int cumulative_time=0;
        if (fs::is_regular_file(path)) {
            std::cout << "File: " << path.u8string() << ", Min size: " << min_size <<" Treshold: "<<ignore_treshold<< "\n";
            processFile(path, min_size, files_with_zeros,buffer_size,ignore_treshold);
        } else if (fs::is_directory(path)) {
            size_t file_count = countFilesInDirectory(path);
            std::cout << "Directory: " << path.u8string() <<
                      ", total files=" << file_count << "\n\n";

            for (auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
                try {
                    if (fs::is_regular_file(entry.path())) {
                        auto t1=std::chrono::high_resolution_clock::now();
                        processFile(entry.path(), min_size, files_with_zeros,buffer_size,ignore_treshold);
                        auto t2=std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1);
                        int ms = static_cast<int>(duration.count());
                        cumulative_time+=ms;
                        file_current_count++;
                        std::cout<<file_current_count<<"/"<<file_count<<" Current: "<<ms<<" ms. Total: "<<cumulative_time<<" ms.          "<<"\r";
                    }
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "Skipping file due to filesystem error: " << e.what() << "\n";
                }
            }
            std::cout<<"Total time: "<<cumulative_time<<std::endl;
        } else {
            std::cerr << "Unsupported file type: " << path.u8string() << "\n";
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    CoInitialize(nullptr);

    std::filesystem::path folder = "Shortcuts";

    if (!std::filesystem::exists(folder))
        std::filesystem::create_directory(folder);

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_or_folder> [min_size] [ignore_treshold] [buffer_size]\n";
        return 1;
    }

    uint64_t min_size = 50000;
    if (argc >= 3) {
        min_size = std::strtoull(argv[2], nullptr, 0);
        if (min_size == 0) min_size = 50000;
    }

    uint64_t ignore_treshold = 1000000;
    if (argc >= 4) {
        ignore_treshold=std::strtoull(argv[3],nullptr,0);
        if (ignore_treshold==0) ignore_treshold=1000000;
    }

    uint64_t buffer_size = 1024*1024*0.1;
    if (argc >= 5) {
        buffer_size=std::strtoull(argv[4],nullptr,0);
        if (buffer_size==0) buffer_size=1024*1024*0.1;
    }

    fs::path path(argv[1]);
    std::set<fs::path> files_with_zeros;

    processPath(path, min_size, files_with_zeros,buffer_size,ignore_treshold);

    if (!files_with_zeros.empty()) {
        std::cout << "\n=== Files containing zero chunks ===\n";
        for (const auto& f : files_with_zeros) {
            std::cout << f.u8string() << "\n";
        }
    } else {
        std::cout << "\nNo zero chunks found in any file.\n";
    }

    CoUninitialize();
    return 0;
}
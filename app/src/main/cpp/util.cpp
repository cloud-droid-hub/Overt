//
// Created by lxz on 2025/6/6.
//

#include "util.h"


std::string get_line(int fd) {
    char buffer;
    std::string line = "";
    while (true) {
        ssize_t bytes_read = nonstd::read(fd, &buffer, sizeof(buffer));
        if (bytes_read == 0) break;
        line += buffer;
        if (buffer == '\n') break;
    }
    return line;
}

std::vector<std::string> get_file_lines(std::string path){
    std::vector<std::string> file_lines = std::vector<std::string>();
    int fd = nonstd::open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return file_lines;
    }
    while (true) {
        std::string line = get_line(fd);
        if (line.size() == 0) {
            break;
        }
        file_lines.push_back(line);
    }
    nonstd::close(fd);
    return file_lines;
}
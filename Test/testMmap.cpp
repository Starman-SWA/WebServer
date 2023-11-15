#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstddef>
#include <cerrno>
#include <iostream>

#define FILESIZE 214

int main(void) {
//    int file = open("wt.txt", O_RDWR | O_CREAT);
//
//    ftruncate(file, FILESIZE);
//
//    char *base = static_cast<char*>( mmap(nullptr,
//                               FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0));

    std::size_t content_length = 214;

    int fd = open("./resources/t", O_RDWR|O_CREAT);
    if (fd == -1) {
        return 1;
    }

    ftruncate(fd, static_cast<__off_t>(content_length));

    char* base = static_cast<char*>(mmap(nullptr, content_length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));

    printf("base = %#x\n", base);
    std::cout << errno;

    munmap(base, FILESIZE);
    return 0;
}
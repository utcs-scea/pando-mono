// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
int main()
{
    int fd = open("file.txt", O_RDONLY, 0);
    if (fd <0 ) {
        printf("open failed: %s\n", strerror(errno));
        return 1;
    }
    struct stat st;
    int x = fstat(fd, &st);
    if (x < 0) {
        printf("fstat failed: %s\n", strerror(errno));
        return 1;
    }
    printf("st_size = %ld\n", st.st_size);
    char buf[1024];
    int n = read(fd, buf, sizeof(buf));
    if (n < 0) {
        printf("read failed: %s\n", strerror(errno));
        return 1;
    }
    printf("read %d bytes\n", n);
    printf("buf = \"%s\"", buf);
    close(fd);

    fd = open("ofile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        printf("open failed: %s\n", strerror(errno));
        return 1;
    }
    write(fd, buf, n);
    close(fd);
    return 0;
}

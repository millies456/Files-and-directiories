#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <stdbool.h>
#include <ctype.h>
#define BUFSIZE 4096

int copy_file(const char *src_path, const char *dst_path) {
    FILE *src_file, *dst_file;
    char buffer[BUFSIZE];
    size_t bytes_read, bytes_written;
    int ret = 0;
    bool success = true;
    
    if (strcmp(src_path, dst_path) == 0) {
        printf("Error: source and destination paths are the same\n");
        return 1;
    }
    
    src_file = fopen(src_path, "rb");
    if (src_file == NULL) {
        printf("Error: cannot open source file '%s': %s\n", src_path, strerror(errno));
        return 1;
    }
    
    dst_file = fopen(dst_path, "rb");
    if (dst_file != NULL) {
        int overwrite = 0;
        char answer[10];
        fclose(dst_file);
        printf("Destination file '%s' already exists. Overwrite? (y/n): ", dst_path);
        fgets(answer, 10, stdin);
        if (tolower(answer[0]) == 'y') {
            dst_file = fopen(dst_path, "wb");
            overwrite = 1;
        }
        else {
            ret = 1;
            success = false;
        }
    }
    else {
        dst_file = fopen(dst_path, "wb");
    }
    
    while (success && (bytes_read = fread(buffer, 1, BUFSIZE, src_file)) > 0) {
        bytes_written = fwrite(buffer, 1, bytes_read, dst_file);
        if (bytes_written != bytes_read) {
            printf("Error: cannot write to destination file '%s': %s\n", dst_path, strerror(errno));
            ret = 1;
            success = false;
        }
    }
    
    if (success) {
        printf("Copied '%s' to '%s'\n", src_path, dst_path);
    }
    
    fclose(src_file);
    fclose(dst_file);
    
    return ret;
}int move_file(const char *src_path, const char *dst_path) {
    int ret = 0;
    
    if (strcmp(src_path, dst_path) == 0) {
        printf("Error: source and destination paths are the same\n");
        return 1;
    }
    
    if (link(src_path, dst_path) == 0) {
        if (unlink(src_path) != 0) {
            printf("Warning: cannot delete source file '%s': %s\n", src_path, strerror(errno));
        }
        printf("Moved '%s' to '%s'\n", src_path, dst_path);
    }
    else {
        printf("Linking failed: %s\n", strerror(errno));
        ret = copy_file(src_path, dst_path);
        if (ret == 0) {
            if (unlink(src_path) != 0) {
                printf("Warning: cannot delete source file '%s': %s\n", src_path, strerror(errno));
            }
            printf("Moved '%s' to '%s'\n", src_path, dst_path);
        }
    }
    
    return ret;
}

int main(int argc, char* argv[]) {
    if(argc < 4) {
        printf("Invalid arguments.\n");
        printf("Usage: %s source1 [source2 ...] destination\n", argv[0]);
        return 1;
    }
    char* command = basename(argv[0]);
    bool is_move = strcmp(command, "move") == 0;
    char* destination = argv[argc - 1];
    struct stat sb;
    stat(destination, &sb);
    if(!S_ISDIR(sb.st_mode) && !S_ISBLK(sb.st_mode)) {
        printf("Invalid destination: %s is not a directory or device.\n", destination);
        return 1;
    }
    int num_files = argc - 2;
    char* files[num_files];
    for(int i = 1; i <= num_files; i++) {
        char* file = argv[i];
        if(stat(file, &sb) != 0) {
            printf("File %s does not exist.\n", file);
            continue;
        }
        char* file_base = basename(file);
        char dest_path[strlen(destination) + strlen(file_base) + 2];
        strcpy(dest_path, destination);
        strcat(dest_path, "/");
        strcat(dest_path, file_base);
        if(is_move) {
            if(link(file, dest_path) == 0) {
                unlink(file);
                printf("Moved '%s' to '%s'\n", file, dest_path);
                continue;
            }
        }
        if(access(dest_path, F_OK) == 0) {
            printf("File %s already exists in destination. Overwrite? (y/n) ", dest_path);
            char overwrite;
            scanf(" %c", &overwrite);
            if(overwrite != 'y' && overwrite != 'Y') {
                continue;
            }
        }
        FILE* src_file = fopen(file, "r");
        FILE* dest_file = fopen(dest_path, "w");
        char buffer[BUFSIZ];
        size_t bytes;
        while((bytes = fread(buffer, 1, BUFSIZ, src_file))) {
            fwrite(buffer, 1, bytes, dest_file);
        }
        fclose(src_file);
        fclose(dest_file);
        if(is_move) {
            unlink(file);
            printf("Moved '%s' to '%s'\n", file, dest_path);
        }
    }
    return 0;
}

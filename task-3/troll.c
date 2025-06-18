#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

const char *troll_path = "/mnt/troll";
const char *trap_flag_path = "/tmp/troll_triggered-2";

const char *troll_ascii = 

" _            _                                                          \n"
" |_ _  | |   _|_ _  ._   o _|_    _.  _   _. o ._    ._ _        _. ._ _| \n"
" | (/_ | |    | (_) |    |  |_   (_| (_| (_| | | |   | (/_ \\/\\/ (_| | (_| \n"
"                                      _|                                  \n";

const char *isDain = "Very spicy internal developer information: leaked roadmap.docx\n";
const char *isNotDain = "DainTontas' personal secret!!.txt\n";
const char *upload_fileMedia = "";

const char* getUSN(){
    struct fuse_context *uid = fuse_get_context();
    struct passwd *pw = getpwuid(uid->uid);
    return pw ? pw->pw_name : "unknown";
}

int trap_triggered(){
    return access(trap_flag_path, F_OK) == 0;
}

void activate(){
    FILE *f = fopen(trap_flag_path, "w");
    if (f){
        fclose(f);
    }
}

static int getattr(const char *path, struct stat *stbuf){
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0){
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if (strcmp(path, "/very_spicy_info.txt") == 0 || strcmp(path, "/upload.txt") == 0){
        stbuf->st_mode = S_IFREG| 0666;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024; // dummy size
    }
    else{
        return -ENOENT;
    }

    return 0;
}

static int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0){
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, "very_spicy_info.txt", NULL, 0);
    filler(buf, "upload.txt", NULL, 0);
    return 0;
}

static int openTroll(const char *path, struct fuse_file_info *fi){
    if (strcmp(path, "/very_spicy_info.txt") != 0 && strcmp(path, "/upload.txt") != 0){
        return -ENOENT;
    }
    return 0;
}

static int readTroll(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    (void) fi;
    const char *username = getUSN();
    const char *fileMedia = "";

    if (trap_triggered()){
        fileMedia = troll_ascii;
    }
    else if (strcmp(path, "/very_spicy_info.txt") == 0){
        if (strcmp(username, "DainTontas") == 0){
            fileMedia = isDain;
        }
        else{
            fileMedia = isNotDain;
        }
    }
    else if (strcmp(path, "/upload.txt") == 0){
        fileMedia = upload_fileMedia;
    }
    else{
        return -ENOENT;
    }

    size_t len = strlen(fileMedia);
    if (offset >= len){
        return 0;
    }

    if (offset + size > len){
        size = len - offset;
    }
    memcpy(buf, fileMedia + offset, size);
    return size;
}

static int writeTroll(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    (void) fi;
    if (strcmp(path, "/upload.txt") == 0 && strcmp(getUSN(), "DainTontas") == 0){
        if (strstr(buf, "upload") != NULL){
            activate();
        }
        return size;
    }
    return -EACCES;
}

static struct fuse_operations DainTontas_Operation = {
    .write = writeTroll,
    .read = readTroll,
    .open = openTroll,
    .getattr = getattr,
    .readdir = readdir,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &DainTontas_Operation, NULL);
}

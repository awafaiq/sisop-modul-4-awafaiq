#define FUSE_USE_VERSION 28

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

const char *mountPath = "/mnt/troll";
const char *trapPath = "/tmp/troll_triggered-2";
const char *isDain = "Very spicy internal developer information: leaked roadmap.docx\n";
const char *isNotDain = "DainTontas' personal secret!!.txt\n";
const char *upload_fileMedia = "";
const char *asciiTroll = 

" _            _                                                          \n"
" |_ _  | |   _|_ _  ._   o _|_    _.  _   _. o ._    ._ _        _. ._ _| \n"
" | (/_ | |    | (_) |    |  |_   (_| (_| (_| | | |   | (/_ \\/\\/ (_| | (_| \n"
"                                      _|                                  \n";

const char* getUSN(){
    struct fuse_context *uid = fuse_get_context();
    struct passwd *userInfo = getpwuid(uid->uid);
    return userInfo ? userInfo->pw_name : "unknown";
}

int trap_triggered(){
    return access(trapPath, F_OK) == 0;
}

void activate(){
    FILE *file = fopen(trapPath, "w");
    if (file){
        fclose(file);
    }
}

static int getattr(const char *path, struct stat *fileInfo){
    memset(fileInfo, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0){
        fileInfo->st_mode = S_IFDIR | 0755;
        fileInfo->st_nlink = 2;
    }
    else if (strcmp(path, "/very_spicy_info.txt") == 0 || strcmp(path, "/upload.txt") == 0){
        fileInfo->st_mode = S_IFREG| 0666;
        fileInfo->st_nlink = 1;
        fileInfo->st_size = 512;
    }
    else{
        return -ENOENT;
    }
    return 0;
}

static int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    // copied from module
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
    // copied from module
    (void) fi;

    const char *username = getUSN();
    const char *fileMedia = "";

    if (trap_triggered()){
        fileMedia = asciiTroll;
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

    size_t length = strlen(fileMedia);
    
    if (offset >= length){
        return 0;
    }

    if (offset + size > length){
        size = length - offset;
    }
    memcpy(buf, fileMedia + offset, size);
    return size;
}

static int writeTroll(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    // copied from module
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

int main(int argc, char *argv[]){
    return fuse_main(argc, argv, &DainTontas_Operation, NULL);
}

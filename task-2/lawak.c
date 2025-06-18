#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>

#define PATH_BUFFER 5000
#define NAME_BUFFER 2000

// Ganti ini dengan folder sumber asli tempat file-file disimpan
#define SOURCE_DIR "/home/naurazz/lawakfs/src"  

// Fungsi ini digunakan untuk menggabungkan path relatif dari FUSE
// dengan direktori sumber di sistem file sebenarnya
static void build_full_path(char full_path[PATH_BUFFER], const char *rel_path) {
    snprintf(full_path, PATH_BUFFER, "%s%s", SOURCE_DIR, rel_path);
}

// Fungsi ini menghapus ekstensi file dari sebuah nama file
// Misalnya "data.txt" menjadi "data"
static void strip_extension(const char *filename, char *stripped) {
    strcpy(stripped, filename);
    char *dot = strrchr(stripped, '.');  // cari titik terakhir
    if (dot != NULL) {
        *dot = '\0';  // hapus ekstensi
    }
}

// Fungsi ini dipanggil saat FUSE ingin tahu info file (ukuran, izin, dll)
static int lawak_getattr(const char *path, struct stat *stbuf)
{
    int result;
    char full_path[PATH_BUFFER];
    build_full_path(full_path, path);  

    result = lstat(full_path, stbuf);  
    if (result == -1) return -errno;

    return 0;
}

// Fungsi ini dipanggil saat pengguna membuka folder di sistem FUSE
// Daftar nama file akan ditampilkan tanpa ekstensi
static int lawak_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    DIR *dir_ptr;
    struct dirent *dir_entry;
    (void) offset;
    (void) fi;

    char full_path[PATH_BUFFER];
    build_full_path(full_path, path);

    dir_ptr = opendir(full_path); 
    if (dir_ptr == NULL) return -errno;

    // baca satu per satu isi direktori
    while ((dir_entry = readdir(dir_ptr)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = dir_entry->d_ino;
        st.st_mode = dir_entry->d_type << 12;

        // hapus ekstensi file sebelum ditampilkan
        char name_without_ext[NAME_BUFFER];
        strip_extension(dir_entry->d_name, name_without_ext);

        // tampilkan ke FUSE tanpa ekstensi
        if (filler(buf, name_without_ext, &st, 0)) break;
    }

    closedir(dir_ptr);
    return 0;
}

// Fungsi ini dipanggil saat pengguna mencoba membuka file
// File dicocokkan berdasarkan nama tanpa ekstensi
static int lawak_open(const char *path, struct fuse_file_info *fi)
{
    char full_path[PATH_BUFFER];
    build_full_path(full_path, path);  
  
    // ambil bagian nama file dan direktori
    char dir_only[PATH_BUFFER], *filename_only;
    strcpy(dir_only, full_path);
    filename_only = strrchr(dir_only, '/');
    if (filename_only == NULL) return -ENOENT;
    *filename_only = '\0';
    filename_only++;

    DIR *dir_ptr = opendir(dir_only);
    if (dir_ptr == NULL) return -errno;

    struct dirent *dir_entry;
    char matched_file[PATH_BUFFER] = {0};
    while ((dir_entry = readdir(dir_ptr)) != NULL) {
        char name_no_ext[NAME_BUFFER];
        strip_extension(dir_entry->d_name, name_no_ext);
        if (strcmp(name_no_ext, filename_only) == 0) {
            snprintf(matched_file, PATH_BUFFER, "%s/%s", dir_only, dir_entry->d_name);
            break;
        }
    }
    closedir(dir_ptr);

    if (matched_file[0] == '\0') return -ENOENT;

    int fd = open(matched_file, O_RDONLY);
    if (fd == -1) return -errno;

    fi->fh = fd;  // simpan file descriptor
    return 0;
}

// Fungsi ini dipanggil saat membaca isi file
static int lawak_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    int result = pread(fi->fh, buf, size, offset);  
    if (result == -1) result = -errno;
    return result;
}

// Fungsi ini dipanggil saat file selesai digunakan
static int lawak_release(const char *path, struct fuse_file_info *fi)
{
    close(fi->fh);  
    return 0;
}

// Daftar operasi yang didukung oleh sistem file 
static struct fuse_operations lawak_oper = {
    .getattr = lawak_getattr,
    .readdir = lawak_readdir,
    .open = lawak_open,
    .read = lawak_read,
    .release = lawak_release,
};

// Fungsi utama, menjalankan FUSE
int main(int argc, char *argv[])
{
    umask(0);  // izin default file
    return fuse_main(argc, argv, &lawak_oper, NULL);
}

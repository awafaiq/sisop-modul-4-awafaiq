[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/V7fOtAk7)
|    NRP     |      Name      |
| :--------: | :------------: |
| 5025241020 | Raynald Ramadhani Fachriansyah |
| 5025241038 | Danish Faiq Ibad Yuadi |
| 5025241042 | Naura Rossa Azalia |

# Praktikum Modul 4 _(Module 4 Lab Work)_

</div>

### Daftar Soal _(Task List)_

- [Task 1 - FUSecure](/task-1/)

- [Task 2 - LawakFS++](/task-2/)

- [Task 3 - Drama Troll](/task-3/)

- [Task 4 - LilHabOS](/task-4/)

### Laporan Resmi Praktikum Modul 4 _(Module 4 Lab Work Report)_


Task 2 : (Naura)
## Task 2 : (Naura)

> ### a. Ekstensi File Tersembunyi
> Semua file yang ditampilkan dalam FUSE mountpoint harus ekstensinya disembunyikan.
> ### b. Akses Berbasis Waktu untuk File Secret
> File yang nama dasarnya adalah secret (misalnya, secret.txt, secret.zip) hanya dapat diakses antara pukul 08:00 (8 pagi) dan 18:00 (6 sore) waktu sistem.
> ### c. Filtering Konten Dinamis
> Ketika sebuah file dibuka dan dibaca, isinya harus secara dinamis difilter atau diubah berdasarkan tipe file yang terdeteksi:
>
> File Teks -> Semua kata yang dianggap lawak (case-insensitive) harus diganti dengan kata "lawak".
>
> File Biner -> Konten biner mentah harus ditampilkan dalam encoding Base64 alih-alih bentuk aslinya.
> ### d.  Logging Akses
> Semua operasi akses file yang dilakukan dalam LawakFS++ harus dicatat ke file yang terletak di /var/log/lawakfs.log.
>
> Setiap entri log harus mematuhi format berikut:
>
> [YYYY-MM-DD HH:MM:SS] [UID] [ACTION] [PATH]


#### Kode Lawak.c
```c
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_WORDS 100
#define MAX_PATH 1024

// [e] Variabel global hasil konfigurasi eksternal
char *filter_words[MAX_WORDS];
int filter_word_count = 0;
char secret_name[256] = "secret"; // default, akan di-overwrite dari lawak.conf
int access_start = 8;
int access_end = 18;
char source_dir[MAX_PATH];


// [d] Logging operasi akses
void log_action(const char *action, const char *path) {
    const char *log_path = "/var/log/lawakfs.log";
    FILE *log = fopen(log_path, "a");

    if (!log) {
        fprintf(stderr, "Log error: Cannot open %s\n", log_path);
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    uid_t uid = getuid();

    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] [%d] [%s] %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            uid, action, path);
    fclose(log);
}


// [a] Menghapus ekstensi dari nama file untuk keperluan display
void trim_extension(const char *filename, char *result) {
    strcpy(result, filename);
    char *dot = strrchr(result, '.');
    if (dot) *dot = '\0';
}

// [b] Mengecek apakah file adalah secret (berdasarkan basename)
int is_secret_file(const char *path) {
    const char *p = strrchr(path, '/');
    if (!p) p = path;
    else p++;

    return strncmp(p, secret_name, strlen(secret_name)) == 0 &&
           (p[strlen(secret_name)] == '.' || p[strlen(secret_name)] == '\0');
}

// [b] Mengecek apakah waktu saat ini dalam rentang akses yang diperbolehkan
int is_time_allowed() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    return t->tm_hour >= access_start && t->tm_hour < access_end;
}


// [e] Memuat konfigurasi dari lawak.conf
void load_config() {
    FILE *f = fopen("lawak.conf", "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "FILTER_WORDS=", 13) == 0) {
            char *tok = strtok(line + 13, ",\n");
            while (tok) {
                filter_words[filter_word_count++] = strdup(tok);
                tok = strtok(NULL, ",\n");
            }
        } else if (strncmp(line, "SECRET_FILE_BASENAME=", 22) == 0) {
            sscanf(line + 22, "%s", secret_name);
        } else if (strncmp(line, "ACCESS_START=", 13) == 0) {
            sscanf(line + 13, "%d", &access_start);
        } else if (strncmp(line, "ACCESS_END=", 11) == 0) {
            sscanf(line + 11, "%d", &access_end);
        }
    }
    fclose(f);
}

// Menggabungkan path FUSE dengan source_dir asli
void full_path(char fpath[MAX_PATH], const char *path) {
    snprintf(fpath, MAX_PATH, "%s%s", source_dir, path);
    printf("DEBUG full_path: %s -> %s\n", path, fpath);
}

// [c] Base64 encoding untuk file biner
static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void encode_base64(FILE *in, char *out, size_t outsize) {
    unsigned char buf[3];
    int len;
    size_t outlen = 0;

    while ((len = fread(buf, 1, 3, in)) > 0) {
        if (outlen + 4 >= outsize) break;
        out[outlen++] = b64chars[buf[0] >> 2];
        out[outlen++] = b64chars[((buf[0] & 3) << 4) | ((buf[1] & 0xf0) >> 4)];
        out[outlen++] = (len > 1) ? b64chars[((buf[1] & 0xf) << 2) | ((buf[2] & 0xc0) >> 6)] : '=';
        out[outlen++] = (len > 2) ? b64chars[buf[2] & 0x3f] : '=';
    }
    out[outlen] = '\0';
}

// [c] Filtering kata-kata dalam file teks
void filter_text(char *buf, size_t size) {
    char *newbuf = malloc(size * 2); // alokasi buffer baru
    newbuf[0] = '\0';

    size_t offset = 0;

    for (char *p = buf; *p;) {
        int matched = 0;
        for (int i = 0; i < filter_word_count; ++i) {
            size_t wlen = strlen(filter_words[i]);
            if (strncasecmp(p, filter_words[i], wlen) == 0) {
                strcpy(newbuf + offset, "lawak");
                offset += 5;
                p += wlen;
                matched = 1;
                break;
            }
        }
        if (!matched) {
            newbuf[offset++] = *p++;
        }
    }

    newbuf[offset] = '\0';
    strcpy(buf, newbuf);
    free(newbuf);
}

//
// ==================
// FUSE OPERATIONS
// ==================
//

// [b] Secret file protection
static int x_getattr(const char *path, struct stat *stbuf) {
    if (is_secret_file(path) && !is_time_allowed())
        return -ENOENT;

    char fpath[MAX_PATH];
    full_path(fpath, path);
    int res = lstat(fpath, stbuf);
    if (res == -1) return -errno;

    log_action("GETATTR", path);
    return 0;
}

// [a,b] Listing file (ekstensi disembunyikan, secret disembunyikan di luar jam)
static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;
    char fpath[MAX_PATH];
    full_path(fpath, path);

    dp = opendir(fpath);
    if (!dp) return -errno;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        if (is_secret_file(de->d_name) && !is_time_allowed())
            continue;

        char name[256];
        trim_extension(de->d_name, name);
        filler(buf, name, NULL, 0);
    }

    closedir(dp);
    log_action("READDIR", path);
    return 0;
}

// [b] Akses file hanya saat jam diperbolehkan
static int x_open(const char *path, struct fuse_file_info *fi) {
    if (is_secret_file(path) && !is_time_allowed())
        return -ENOENT;

    char fpath[MAX_PATH];
    full_path(fpath, path);
    int res = open(fpath, O_RDONLY);
    if (res == -1) return -errno;
    close(res);
    log_action("OPEN", path);
    return 0;
}

// [b,d] Validasi akses dan logging
static int x_access(const char *path, int mask) {
    if (is_secret_file(path) && !is_time_allowed())
        return -ENOENT;

    char fpath[MAX_PATH];
    full_path(fpath, path);
    int res = access(fpath, mask);
    if (res == -1) return -errno;

    log_action("ACCESS", path);
    return 0;
}

// [c] Membaca isi file, memfilter teks, base64 untuk biner
static int x_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[MAX_PATH];
    full_path(fpath, path);

    FILE *f = fopen(fpath, "rb");
    if (!f) return -errno;

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    rewind(f);

    char *tmp = malloc(len + 1);
    fread(tmp, 1, len, f);
    tmp[len] = '\0';

    int is_binary = 0;
    for (size_t i = 0; i < len; ++i) {
        if (tmp[i] == '\0' || tmp[i] > 127) {
            is_binary = 1;
            break;
        }
    }

    if (is_binary) {
        rewind(f);
        char *encoded = malloc(len * 2);
        encode_base64(f, encoded, len * 2);
        memcpy(buf, encoded + offset, size);
        free(encoded);
    } else {
        filter_text(tmp, len);
        memcpy(buf, tmp + offset, size);
    }

    free(tmp);
    fclose(f);
    log_action("READ", path);
    return size;
}

// Enforcement Read-Only
static int ro() { return -EROFS; }

static struct fuse_operations x_oper = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .open    = x_open,
    .read    = x_read,
    .access  = x_access,
    .write   = (void *)ro,
    .mkdir   = (void *)ro,
    .unlink  = (void *)ro,
    .rmdir   = (void *)ro,
    .rename  = (void *)ro,
};

// Inisialisasi filesystem dan argumen mount
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mount_point>\n", argv[0]);
        return 1;
    }

    if (!realpath(argv[1], source_dir)) {
        perror("realpath failed");
        return 1;
    }

    load_config(); // [e] Load konfigurasi

    // Pass args ke FUSE
    char *fuse_argv[argc];
    fuse_argv[0] = argv[0];
    for (int i = 2; i < argc; ++i)
        fuse_argv[i - 1] = argv[i];

    return fuse_main(argc - 1, fuse_argv, &x_oper, NULL);
}
```

> ### e. Konfigurasi

#### Kode Lawak.conf
```c
FILTER_WORDS=ducati,ferrari,mu,chelsea,prx,onic,sisop
SECRET_FILE_BASENAME=secret
ACCESS_START=08:00
ACCESS_END=20:00
```
### Task 3 : (Faiq) — Drama Troll

#### a. Pembuatan User

Buat 3 user di sistem sebagai berikut yang merepresentasikan aktor-aktor yang terlibat dalam trolling kali ini, yaitu:

* `DainTontas` → target utama troll.
* `SunnyBolt` dan `Ryeku` → dalang di balik jebakan.

User ditambahkan menggunakan `useradd` dan diberi password via `passwd`.

#### b. Jebakan Troll

Filesystem FUSE dibuat dan dimount ke /mnt/troll. Di dalamnya cuma ada 2 file :

* `very_spicy_info.txt` → umpan berisi informasi "bocoran".
* `upload.txt` → file kosong, tapi menjadi trigger saat di-*echo* oleh DainTontas.

#### c. Jebakan Troll (Berlanjut)

Isi dari `very_spicy_info.txt` bergantung pada siapa yang membuka:

* Jika dibuka oleh **DainTontas** → muncul "leaked roadmap.docx".
* Jika dibuka oleh user lain → muncul "DainTontas' personal secret!!".

Dilakukan dengan cara mendeteksi siapa user yang sedang membaca file-nya.

#### d. Trap

Jika **DainTontas** melakukan `echo upload > upload.txt`, maka file `upload.txt` akan memicu jebakan:

* File `/tmp/troll_triggered-2` akan dibuat (sebagai flag persist).
* Setelah flag aktif, semua pembacaan `.txt` akan memunculkan ASCII art bertuliskan "Fell for it again reward".

Fungsi `write` akan memeriksa isi `upload.txt`, dan jika ada kata "upload" dari user `DainTontas`, trap diaktifkan.

### 1. Header dan Library

```c
#define FUSE_USE_VERSION 28
```

Menggunakan FUSE Versi 28 sesuai dengan modul.

```c
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
```

Mengimpor library yang diperlukan, manipulasi file, user info, dan error handling.

```c
const char *mountPath = "/mnt/troll";
const char *trapPath = "/tmp/troll_triggered-2";
```

Path untuk mount FUSE dan file pemicu jebakan.

```c
const char *isDain = "Very spicy internal developer information: leaked roadmap.docx\n";
const char *isNotDain = "DainTontas' personal secret!!.txt\n";
const char *uploadFileMedia = "";
```

Isi file berdasarkan user dan placeholder untuk upload.txt.

```c
const char *asciiTroll = 

" _            _                                                          \n"
" |_ _  | |   _|_ _  ._   o _|_    _.  _   _. o ._    ._ _        _. ._ _| \n"
" | (/_ | |    | (_) |    |  |_   (_| (_| (_| | | |   | (/_ \\/\\/ (_| | (_| \n"
"                                      _|                                  \n";

```

ASCII Art akan muncul setelah jebakan aktif.

### 2. Fungsi Support

```c
const char* getUSN(){
    struct fuse_context *uid = fuse_get_context();
    struct passwd *userInfo = getpwuid(uid->uid);
    return userInfo ? userInfo->pw_name : "unknown";
}
```

Untuk tahu siapa yang sedang mengakses FUSE.

```c
int triggerTrap(){
    return access(trapPath, F_OK) == 0;
}
```

Cek apakah jebakan sudah aktif (file `/tmp/...` sudah ada).

```c
void activate(){
    FILE *file = fopen(trapPath, "w");
    if (file){
        fclose(file);
    }
}
```

Aktifkan jebakan: buat file sebagai penanda bahwa jebakan sudah aktif

### 3. Fungsi getattr

```c
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
```

1. Inisialisasi struktur `fileInfo` menjadi 0.
2. Jika path root, set sebagai direktori.
3. Jika salah satu dari dua file, set sebagai file reguler.
4. Jika path tidak dikenal, kembalikan `error ENOENT`.

### 4. Fungsi readdir

```c
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

```

1. Cast `fi` dan `offset` ke void.
2. Cek hanya direktori root yang dapat diakses.
3. Tambahkan entri direktori ke buffer.

### Fungsi open

```c
static int openTroll(const char *path, struct fuse_file_info *fi){
    if (strcmp(path, "/very_spicy_info.txt") != 0 && strcmp(path, "/upload.txt") != 0){
        return -ENOENT;
    }
    return 0;
}
```

Hanya izinkan membuka file jika path sesuai.

### 5. Fungsi read

```c
static int readTroll(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    // copied from module
    (void) fi;

    const char *username = getUSN();
    const char *fileMedia = "";

    if (triggerTrap()){
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
        fileMedia = uploadFileMedia;
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
```

1. Cast `fi` ke void.
2. Ambil username dan siapkan isi file.
3. Jika jebakan aktif, tampilkan ASCII Art.
4. Isi file berubah tergantung user.
5. Cek file yang lain dan kembalikan error jika tidak dikenal.
6. Salin konten ke buffer dengan batasan offset dan size.

### 6. FUSE Callback: write

```c
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

```

1. Cast `fi` ke void.
2. Jika user dan konten sesuai, jebakan diaktifkan.

### 7. Fungsi FUSE Bind

```c
static struct fuse_operations DainTontas_Operation = {
    .write = writeTroll,
    .read = readTroll,
    .open = openTroll,
    .getattr = getattr,
    .readdir = readdir,
};
```

Menghubungkan fungsi-fungsi dengan operasi FUSE.

### 8. Fungsi Main

```c
int main(int argc, char *argv[]){
    return fuse_main(argc, argv, &DainTontas_Operation, NULL);
}
```

Menjalankan filesystem dengan operasi yang telah didefinisikan.





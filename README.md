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

### Task 1 : (Yuadi) — FUSecure

#### a. Pembuatan User dan Struktur Folder

Buat 2 user di sistem yang akan berperan sebagai pemilik data:

* `yuadi` → pemilik folder `private_yuadi`
* `irwandi` → pemilik folder `private_irwandi`

Struktur folder yang digunakan dalam source directory `/home/shared_files`:

* `public/` - folder umum yang bisa diakses siapa pun
* `private_yuadi/` - hanya bisa dibaca oleh user `yuadi`
* `private_irwandi/` - hanya bisa dibaca oleh user `irwandi`

User ditambahkan menggunakan `useradd` dan diberi password via `passwd`.

#### b. FUSE Mount Point

Sistem file FUSE dimount ke `/mnt/secure_fs` dan akan meng-copy/reflect isi dari `/home/shared_files`.

Jika kita `ls /mnt/secure_fs`, maka akan muncul:
* `public/`
* `private_yuadi/`
* `private_irwandi/`
* 
![sisop-modul4-1-b](https://github.com/user-attachments/assets/11f79c44-1c3a-4f4f-bfe7-01e3fa4842ae)


#### c. Read-Only System

Seluruh sistem FUSE bersifat read-only, artinya:

* Tidak ada user (termasuk root) yang bisa melakukan `mkdir`, `rmdir`, `touch`, `rm`, `cp`, `mv`, atau memodifikasi file apa pun.
* Semua operasi write atau modify ditolak dengan `Permission denied`.

#### d. Akses ke Folder Public

Semua user dapat membaca file dari folder `public`.

Misal: 
```bash
cat /mnt/secure_fs/public/materi_algoritma.txt
```

Akan berhasil untuk user manapun, termasuk `yuadi`, `irwandi`, bahkan user lain.
![sisop-modul4-1-c](https://github.com/user-attachments/assets/69a821f7-a35f-44fc-8eca-a4f0dbf3ec8d)


#### e. Akses ke Folder Private

* File dalam `private_yuadi/` hanya dapat dibaca oleh user `yuadi`.
* File dalam `private_irwandi/` hanya dapat dibaca oleh user `irwandi`.

Jika user lain mencoba membaca file di folder yang bukan miliknya, maka akan gagal dengan `Permission denied`.
![sisop-modul4-1-e](https://github.com/user-attachments/assets/8961f79e-704f-47cb-a38e-f8b415b0039e)


### 1. Header dan Library

```c
#define FUSE_USE_VERSION 28
```

Gunakan FUSE versi 28 sesuai modul.

```c
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
```

Mengimpor semua library yang dibutuhkan untuk operasi file dan direktori.

```c
static const char *source_path = "/home/shared_files";
```

Menentukan direktori tempat data sebenarnya disimpan.

### 2. Fungsi Support

```c
void mkdir_mountpoint(const char *path)
```

Membuat folder mount point jika belum ada.

```c
static int checkAccessPerm(const char *path, uid_t uid, gid_t gid) {
    char fullPath[PATH_MAX];
    struct stat st;
  
    snprintf(fullPath, sizeof(fullPath), "%s%s", source_path, path);
    if (lstat(fullPath, &st) == -1)
      return -errno;
  
    const char *yuadi_path = "/private_yuadi";
    const char *irwandi_path = "/private_irwandi";
  
    fprintf(stderr, "path=%s, uid=%d, st_uid=%d\n", path, uid, st.st_uid);
  
    if ((strncmp(path, yuadi_path, strlen(yuadi_path)) == 0 &&
         (path[strlen(yuadi_path)] == '/' || path[strlen(yuadi_path)] == '\0'))) {
      if (uid != st.st_uid)
        return -EACCES;
    }
  
    if ((strncmp(path, irwandi_path, strlen(irwandi_path)) == 0 &&
         (path[strlen(irwandi_path)] == '/' ||
          path[strlen(irwandi_path)] == '\0'))) {
      if (uid != st.st_uid)
        return -EACCES;
    }
    return 0;
}
```

Cek apakah user berhak mengakses file berdasarkan path:
- `/private_yuadi/` - hanya UID 1001 (`yuadi`)
- `/private_irwandi/` - hanya UID 1002 (`irwandi`)
- `public/` -  bebas akses

### 3. Fungsi getattr

```c
static int xmp_getattr(const char *path, struct stat *stbuf) {
  int res;
  char fpath[PATH_MAX];

  sprintf(fpath, "%s%s", source_path, path);

  res = lstat(fpath, stbuf);

  if (res == -1)
    return -errno;

  return 0;
}
```

1. Bangun path absolut file di source directory.
2. Ambil informasi metadata file menggunakan `lstat`.
3. Kembalikan data ke FUSE.

### 4. Fungsi readdir

```c
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
  char fpath[PATH_MAX];

  if (strcmp(path, "/") == 0) {
    path = source_path;
    sprintf(fpath, "%s", path);
  } else {
    sprintf(fpath, "%s%s", source_path, path);
  }

  int res = 0;

  DIR *dp;
  struct dirent *de;
  (void)offset;
  (void)fi;

  dp = opendir(fpath);

  if (dp == NULL)
    return -errno;

  while ((de = readdir(dp)) != NULL) {
    char childPath[PATH_MAX];

    snprintf(childPath, sizeof(childPath), "%s%s", path, de->d_name);
    struct stat st;

    memset(&st, 0, sizeof(st));

    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    res = (filler(buf, de->d_name, &st, 0));

    if (res != 0)
      break;
  }

  closedir(dp);

  return 0;
}
```

1. Menerjemahkan direktori FUSE ke direktori nyata (`source_path + path`).
2. Menggunakan `opendir` dan `readdir` untuk membaca isi direktori.
3. Mengisi buffer FUSE dengan nama-nama file dan folder yang ditemukan.

### 5. Fungsi read

```c
static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  char fpath[PATH_MAX];
  if (strcmp(path, "/") == 0) {
    path = source_path;

    sprintf(fpath, "%s", path);
  } else {
    sprintf(fpath, "%s%s", source_path, path);

    int accessGrant =
        checkAccessPerm(path, fuse_get_context()->uid, fuse_get_context()->gid);
    if (accessGrant != 0)
      return accessGrant;
  }

  int res = 0;
  int fd = 0;

  (void)fi;

  fd = open(fpath, O_RDONLY);

  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);

  if (res == -1)
    res = -errno;

  close(fd);

  return res;
}
```

1. Periksa apakah user boleh membaca file tersebut (`checkAccessPerm()`).
2. Jika diizinkan, buka file dengan `open`.
3. Baca isinya ke buffer menggunakan `pread`.

### 6. Fungsi open

```c
static int xmp_open(const char *path, struct fuse_file_info *fi) {
  char fullPath[PATH_MAX];

  snprintf(fullPath, sizeof(fullPath), "%s%s", source_path, path);

  if ((fi->flags & O_ACCMODE) != O_RDONLY)
    return -EACCES;

  int accessGrant =
      checkAccessPerm(path, fuse_get_context()->uid, fuse_get_context()->gid);
  if (accessGrant != 0)
    return accessGrant;

  int res = open(fullPath, fi->flags);
  if (res == -1)
    return -errno;

  close(res);
  return 0;
}
```

1. Pastikan file dibuka dalam mode read-only (`O_RDONLY`).
2. Gunakan `checkAccessPerm()` untuk izin akses.
3. Kembalikan error jika tidak diizinkan.

### 7. Fungsi-Fungsi yang Ditolak (Write, Modify)

Semua fungsi yang memodifikasi file ditolak dengan:

```c
return -EACCES;
```

Fungsi yang ditolak antara lain:
* `xmp_write`
* `xmp_create`
* `xmp_unlink`
* `xmp_mkdir`
* `xmp_rmdir`
* `xmp_rename`
* `xmp_truncate`
* `xmp_utimens`
* `xmp_chmod`
* `xmp_chown`

### 8. Registrasi Operasi FUSE

```c
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read    = xmp_read,
    .open    = xmp_open,
    .write   = xmp_write,
    ...
};
```

1. Menghubungkan semua fungsi handler ke operasi FUSE yang sesuai.

### 9. Fungsi Main

```c
int main(int argc, char *argv[]) {
    umask(0);
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <mountpoint> [-o allow_other] [-f] [-d]
", argv[0]);
        return 1;
    }
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
```

1. Jalankan FUSE dengan argumen yang diberikan.
2. Pastikan `-o allow_other` digunakan agar semua user bisa mengakses.

### Contoh Penggunaan
![Screenshot from 2025-06-22 20-38-22](https://github.com/user-attachments/assets/e24d6bbe-eb8e-42ca-872e-2a5451fe7f19)
note : Ada sedikit kesalahan pada akses private_yuadi serta private_irwandi ketika mencoba sisop.txt

Kesulitan : seperti yang sudah saya sampaikan pada contoh penggunaan, sampai pada detik terakhir revisi, saya masih kesulitan untuk menemukan problemnya pada bagian mana. ;_;


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

### Penjelasan Lawak.c

[e] Variabel global hasil konfigurasi eksternal
```c
char *filter_words[MAX_WORDS];
int filter_word_count = 0;
char secret_name[256] = "secret"; // default, akan di-overwrite dari lawak.conf
int access_start = 8;
int access_end = 18;
char source_dir[MAX_PATH];
```

- `char *filter_words[MAX_WORDS];`
Array pointer ke kata-kata yang akan difilter dari file teks (misalnya: "ducati", "mu")

- `int filter_word_count = 0;`
Menyimpan jumlah kata dalam filter_words[]

- `char secret_name[256] = "secret";`
Nama file yang akan dianggap sebagai "secret". Nilai default adalah "secret", tetapi bisa diubah lewat lawak.conf (parameter SECRET_FILE_BASENAME)

- `int access_start = 8;`
Batas waktu mulai untuk akses file "secret". Default: jam 8 pagi (08:00)

- `int access_end = 18;`
Batas waktu akhir untuk akses file "secret". Default: jam 6 sore (18:00)

`char source_dir[MAX_PATH];`
- Direktori sumber data yang akan dipasangkan dengan sistem file virtual ini


[d] Logging Operasi Akses
```c
void log_action(const char *action, const char *path) {
    const char *log_path = "/var/log/lawakfs.log";
    FILE *log = fopen(log_path, "a"); // membuka file log dalam mode append

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

    fclose(log); // tutup file log
}
```

- `const char *log_path = "/var/log/lawakfs.log";`
- Path lokasi log disimpan. File ini berada di sistem /var/log/, jadi butuh izin root untuk menulis ke sana.

- `FILE *log = fopen(log_path, "a");`
Membuka file log dalam mode append (menambahkan baris baru tanpa menghapus sebelumnya).

- `if (!log) {...}`
Jika file tidak bisa dibuka (misalnya karena tidak ada izin), maka tampilkan error ke stderr, lalu keluar dari fungsi.

- `time_t now = time(NULL);`
Mengambil waktu saat ini dalam bentuk epoch time (jumlah detik sejak 1970).

- `struct tm *t = localtime(&now);`
Mengubah epoch time ke struktur waktu lokal (struct tm), supaya bisa dipecah jadi tahun, bulan, tanggal, jam, menit, detik.

- `uid_t uid = getuid();`
Mendapatkan User ID (UID) dari user yang sedang menjalankan program ini.

- `fprintf(log, "...", ...)`

Menuliskan log ke file dalam format berikut:
```
[YYYY-MM-DD HH:MM:SS] [UID] [ACTION] /path/to/file
```
Contoh log:
```
[2025-06-22 17:02:11] [1000] [READ] /data/secret.txt
```
- `fclose(log);`
Menutup file log agar tidak terjadi kebocoran file descriptor.

[a] Menghapus Ekstensi dari Nama File
```c
void trim_extension(const char *filename, char *result) {
    strcpy(result, filename);
    char *dot = strrchr(result, '.');
    if (dot) *dot = '\0';
}
```

- `strcpy(result, filename);`
Salin isi filename ke buffer result. Ini penting agar kita bisa memodifikasi result tanpa merusak input asli.

- `char *dot = strrchr(result, '.');`
Mencari posisi terakhir titik ('.') dalam nama file.
Fungsi strrchr mencari dari belakang, sehingga cocok untuk menemukan ekstensi file seperti .txt, .jpg, dll.

- `if (dot) *dot = '\0';`
Jika titik ditemukan (berarti file punya ekstensi), maka kita “memotong” string tepat di titik tersebut dengan mengubahnya menjadi null-terminator ('\0').
Ini membuat hanya nama tanpa ekstensi yang tersisa di result.

[b] Proteksi File Secret Berdasarkan Waktu
1. `is_secret_file(const char *path)`
```c
int is_secret_file(const char *path) {
    const char *p = strrchr(path, '/');
    if (!p) p = path;
    else p++;

    return strncmp(p, secret_name, strlen(secret_name)) == 0 &&
           (p[strlen(secret_name)] == '.' || p[strlen(secret_name)] == '\0');
}
```

- `const char *p = strrchr(path, '/');`
Mencari karakter / terakhir dalam path untuk mengambil basename (nama file saja, tanpa folder).

- `if (!p) p = path; else p++;`
Jika tidak ada /, berarti path itu langsung nama file (misal "secret.txt"). Jika ada, p digeser satu karakter ke kanan untuk mulai dari nama file-nya.

- `strncmp(p, secret_name, strlen(secret_name)) == 0`
Mengecek apakah nama file dimulai dengan secret_name (default "secret"). Jadi secret.txt, secret.png, dan secret akan cocok.

- `(p[strlen(secret_name)] == '.' || p[strlen(secret_name)] == '\0')`
Pastikan setelah nama `secret`, hanya boleh:

    - Titik (`.`) -> berarti ekstensi, atau

    - Null terminator -> berarti nama file pas `"secret"` tanpa ekstensi.

2. `is_time_allowed()`
```c
int is_time_allowed() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    return t->tm_hour >= access_start && t->tm_hour < access_end;
}
```
- Mengambil waktu sekarang (`time(NULL) ` dan `localtime`) lalu periksa apakah jam saat ini masih dalam rentang access_start s.d. `access_end`.

[c] Filtering Isi File Teks & Encode Base64 untuk File Biner

1. `filter_text(char *buf, size_t size)`
```c
void filter_text(char *buf, size_t size) {
    char *newbuf = malloc(size * 2);
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
```
- `malloc(size * 2)` -> Alokasi buffer baru 2x ukuran teks asli (untuk jaga-jaga setelah sensor).

- Loop utama: membaca karakter demi karakter dari `buf`.

Jika ditemukan kata dari `filter_words[]`, maka diganti dengan `"lawak"`.

- Copy hasil akhirnya ke buffer asli `buf`.

Contoh:

File berisi: `aku suka ferrari dan mu`

Setelah filter: `aku suka lawak dan lawak`

2. `encode_base64(FILE *in, char *out, size_t outsize)`
```c
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
```
- Membaca input file 3 byte per blok.

- Mengubahnya ke format base64 dengan 4 karakter.

- Jika kurang dari 3 byte, diberi padding `=`.

Base64 digunakan karena FUSE tidak bisa menampilkan byte biner mentah secara aman — dengan encoding, semua karakter menjadi ASCII printable.

3. Digunakan di `x_read()`
```c
// membaca file
FILE *f = fopen(fpath, "rb");
fseek(f, 0, SEEK_END);
size_t len = ftell(f);
rewind(f);
char *tmp = malloc(len + 1);
fread(tmp, 1, len, f);

// deteksi biner
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
```

- File dibaca penuh ke buffer `tmp`.

- Cek apakah ada karakter NULL atau non-ASCII (di atas 127) -> kalau iya → file dianggap biner.

- Jika biner -> encode base64.

- Jika teks -> lakukan filter kata.

- Setelah diproses, hasilnya dicopy ke `buf`, memperhitungkan `offset`.

**Ringkasan:**

- Teks -> difilter (misalnya “mu” jadi “lawak”)

- Biner -> base64 supaya aman ditampilkan

- Filter dan encoding dilakukan on-the-fly saat file dibaca

[e] Memuat Konfigurasi dari lawak.conf

```c
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
```

- `FILE *f = fopen("lawak.conf", "r");`
Membuka file konfigurasi bernama lawak.conf untuk dibaca.

- `if (!f) return;`
Jika file tidak ditemukan atau tidak bisa dibuka, maka abaikan konfigurasi dan gunakan nilai default.

- `while (fgets(line, sizeof(line), f))`
Baca file baris demi baris.

**Parsing Konfigurasi**
1. `FILTER_WORDS=mu,ferrari,onic`
```c
if (strncmp(line, "FILTER_WORDS=", 13) == 0) {
    char *tok = strtok(line + 13, ",\n");
    while (tok) {
        filter_words[filter_word_count++] = strdup(tok);
        tok = strtok(NULL, ",\n");
    }
}
```

- Ambil substring setelah `FILTER_WORDS=`

- Pisahkan berdasarkan koma

- Simpan tiap kata ke `filter_words[]`

- Gunakan `strdup untuk menyimpan salinan string

2. `SECRET_FILE_BASENAME=secret`
```c
else if (strncmp(line, "SECRET_FILE_BASENAME=", 22) == 0) {
    sscanf(line + 22, "%s", secret_name);
}
```

- Ekstrak nama file “secret” yang akan dibatasi aksesnya

- Disimpan ke variabel `secret_name`

3. `ACCESS_START=08`
```c
else if (strncmp(line, "ACCESS_START=", 13) == 0) {
    sscanf(line + 13, "%d", &access_start);
}
```
- Baca jam awal akses file secret

4. ACCESS_END=18
```c
else if (strncmp(line, "ACCESS_END=", 11) == 0) {
    sscanf(line + 11, "%d", &access_end);
}
```
- Baca jam akhir akses file secret

## Penjelasan lawak.conf
- `FILTER_WORDS=`
Daftar kata yang akan difilter dari file teks dan diganti dengan "lawak".

- `SECRET_FILE_BASENAME=secret`
Nama file yang dianggap rahasia, hanya bisa diakses di waktu tertentu.

- `ACCESS_START=08:00`
Jam mulai file rahasia boleh diakses (default: 08 pagi).

- `ACCESS_END=18:00`
Jam akhir akses file rahasia (default: 18 sore).

## Screenshot
[a] Ekstensi File Tersembunyi
- jam 10.00 - 20.00
  
![A saat jam 10 00-20 00](https://github.com/user-attachments/assets/55d802b5-650b-4040-8f82-055cd6af9199)
- diluar jam 10.00-20.00
![A saat diluar jam 10 00-20 00](https://github.com/user-attachments/assets/0842da1d-6beb-4a6b-8ea8-2b19c09a9e77)

[b] Akses Berbasis Waktu untuk File Secret
- jam 10.00 - 20.00
  
![secret txt rentang jam](https://github.com/user-attachments/assets/7a4ebc4c-305f-4e53-acce-1387a5a599bc)
- diluar jam 10.00-20.00
![secret txt diluar jam](https://github.com/user-attachments/assets/32c8b334-cc55-46ea-b51c-eb759e1e2cda)

[c] Filtering Konten Dinamis
- File Biner
![kelinci png](https://github.com/user-attachments/assets/ece9f0b0-a09f-4229-8819-a657e17b9b5c)
- File Teks
![readme txt](https://github.com/user-attachments/assets/260f0f40-65d2-4c38-9d3d-69fd3aaabbf1)

[d] Logging Akses
![varlog](https://github.com/user-attachments/assets/bc02ce49-306d-4dee-b7a5-8eeccccdac42)


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

### 7. Registrasi Operasi FUSE

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

### Contoh :
1. su - DainTontas
Sebelum echo :
![Screenshot from 2025-06-22 18-35-32](https://github.com/user-attachments/assets/84ad481f-7e0b-4de5-93c3-d1a8b4c435cb)

Setelah echo : 

![Screenshot from 2025-06-22 18-38-12](https://github.com/user-attachments/assets/ce00d774-8b35-4318-84a0-3c7183447b5f)

2. selain su - DainTontas :
![Screenshot from 2025-06-22 18-36-04](https://github.com/user-attachments/assets/2ed8b24d-02f8-422c-9759-e8fd482ab7cd)



## Task 4 (Raynald)

> Lilhab sedang ditantang oleh Trabowo (orang yang sama yang dia temui di modul ke-1) untuk membuat kernel sederhana yang memiliki fitur piping menggunakan echo, grep, dan wc. Lilhab merasa kesulitan dan gugup karena dia pikir pekerjaannya tidak akan selesai ketika bertemu dengan deadline. Jadi, dia memutuskan untuk bertanya kepada Grok tentang tantangan tersebut dan AI tersebut memutuskan untuk mengejeknya.

### _a. Implementasikan fungsi `printString`, `readString`, dan `clearScreen` di kernel.c yang akan menampilkan dan membaca string di layar._

- `printString`: Menampilkan string yang diakhiri null menggunakan int 10h dengan AH=0x0E.
- `readString`: Membaca karakter dari keyboard menggunakan int 16h dengan AH=0x00 sampai Enter ditekan. Termasuk penanganan Backspace dasar.
- `clearScreen`: Membersihkan layar dan mengatur kursor ke pojok kiri atas (0, 0) menggunakan int 10h dengan AH=0x06 dan AH=0x02. Buffer video untuk warna karakter akan diubah menjadi putih.

  **_1_**. `printstring`

#### Kode

```c
void printString(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        interrupt(0x10, 0x0E00 + str[i], 0, 0, 0);
        i++;
    }
}
```

- while (str[i] != '\0') = Looping `i` sampai mencapai `null`

- interrupt(0x10, 0x0E00 + str[i], 0, 0, 0); = Menggunakan fungsi interrupt dengan format `extern int interrupt(int number, int AX, int BX, int CX, int DX);` dimana:

  - 0x10 -> interrupt untuk video services.
  - 0x0E00 -> AH untuk mencetak karakter ke layar.
  - str[i] -> karakter yang akan ditampilkan.
  - 0, 0, 0 -> BX, CX, DX yang tidak digunakan

  **_2_**. `readstring`

#### Kode

```c
void readString(char* buf) {
    int i = 0;
    char c;

    while (1) {
        c = interrupt(0x16, 0x0000, 0, 0, 0) & 0xFF;

        if (c == 13) { //enter
            buf[i] = '\0';
            interrupt(0x10, 0x0E0D, 0, 0, 0);
            interrupt(0x10, 0x0E0A, 0, 0, 0);
            break;
        } else if (c == 8) { //backspace
            if (i > 0) {
                i--;
                interrupt(0x10, 0x0E08, 0, 0, 0);
                interrupt(0x10, 0x0E20, 0, 0, 0);
                interrupt(0x10, 0x0E08, 0, 0, 0);
            }
        } else {
            buf[i] = c;
            interrupt(0x10, 0x0E00 + c, 0, 0, 0); //echo input
            i++;
        }
    }
}
```

- c = interrupt(0x16, 0x0000, 0, 0, 0) & 0xFF; = Mendeklarasikan c yang diisi dengan fungsi interrupt:

  - 0x16 -> Interrupt untuk keyboard services.
  - 0x0000 -> AH untuk membaca input dari keyboard.
  - 0, 0, 0 -> BX, CX, DX yang tidak digunakan.
  - & 0xFF -> Mengambil byte terakhir dari hasil fungsi interrupt.

- if (c == 13) = Jika input adalah 13 (ASCII code untuk enter), maka:

  - buf[i] = '\0'; -> Mengakhiri string dengan null character.
  - 0x10 -> Interrupt untuk video services.
  - 0x0E0D -> AH untuk mencetak Carriage Return ke layar.
  - 0x0E0A -> AH untuk mencetak Line Feed ke layar.

- else if (c == 8) = Jika input adalah 8 (ASCII code untuk backspace), maka:

  - 0x10 -> Interrupt untuk video services.
  - 0x0E08 -> AH untuk mencetak Backspace ke layar.
  - 0x0E20 -> AH untuk mencetak Spasi ke layar.
  - 0x0E08 -> AH untuk mencetak Backspace ke layar.

- else = Jika input bukan 13 atau 8, maka:

  - buf[i] = c; -> Menyimpan input ke dalam buffer.
  - 0x10 -> Interrupt untuk video services.
  - 0x0E00 + c -> AH untuk mencetak input ke layar.

  **_3_**. `clearScreen`

#### Kode

```c
void clearScreen() {
    interrupt(0x10, 0x0600, 0x0700, 0x0000, 0x184F);
    interrupt(0x10, 0x0200, 0, 0, 0);
}
```

- interrupt:
  - 0x10 -> Interrupt untuk video services.
  - 0x0600 → AH 0x06 (clear screen), AL = 0x00 (clear semua baris).
  - 0x0700 → BH 0x07 (atribut karakter: putih di atas hitam).
  - CX = 0x0000 → baris mulai dari (0,0).
  - DX = 0x184F → baris sampai (24,79) → area layar penuh (25 baris, 80 kolom).
  - 0x0200 → AH 0x02 (set cursor position).

---

### _b. Lengkapi implementasi fungsi-fungsi di std_lib.h dalam std_lib.c._

Fungsi-fungsi di atas dapat digunakan untuk menyederhanakan implementasi fungsi printString, readString, clearScreen, dan fungsi-fungsi lebih lanjut yang dijelaskan pada tugas berikutnya.

**_1_**. `div`

#### Kode

```c
int div(int a, int b) {
    int d = 0;
    while (a >= b) {
        a -= b;
        d++;
    }
    return d;
}
```

- int d = 0; -> Menyimpan hasil akhir (jumlah berapa kali b bisa dikurangkan dari a).
- while (a >= b) -> Loop selama a masih lebih besar atau = b.
- a -= b; -> Kurangkan b dari a.
- d++; -> Tambah penghitung hasil (d) setiap kali pengurangan berhasil.
- return d; -> Kembalikan jumlah pengurangan sebagai hasil pembagian bulat.

**_2_**. `mod`

#### Kode

```c
int mod(int a, int b) {
    while (a >= b) {
        a -= b;
    }
    return a;
}
```

- while (a >= b) -> Loop selama a masih lebih besar atau = b.
- a -= b; -> Kurangkan a dengan b.
- return a; -> Kembalikan nilai a sebagai sisa pembagian.

**_3_**. `memcpy`

#### Kode

```c
void memcpy(byte* src, byte* dst, unsigned int size) {
    unsigned int i;
    for (i = 0; i < size; i++) {
        dst[i] = src[i];
    }
}
```

- loop for -> loop i sampai kurang dari size.
- dst[i] = src[i]; -> salin nilai src ke dst.

**_4_**. `strlen`

#### Kode

```c
unsigned int strlen(char* str) {
    unsigned int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
```

- while (str[len] != '\0') -> Loop selama karakter di str tidak sama dengan '\0.
- len++; -> Increment len.
- return len; -> Kembalikan nilai len sebagai nilai panjang string.

**_5_**. `strcmp`

```c
bool strcmp(char* str1, char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return false;
        }
        i++;
    }
    return (str1[i] == '\0' && str2[i] == '\0');
}
```

- if (str1[i] != str2[i]) -> Jika karakter di str1 dan str2 tidak sama, return false.
- i++; -> Increment i, sebagai penanda posisi karakter.

**_6_**. `strcpy`

#### Kode

```c
void strcpy(char* src, char* dst) {
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}
```

- dst[i] = src[i]; -> Salin nilai src ke dst.
- dst[i] = '\0'; -> Setelah mencapai akhir string, tambahkan '\0' untuk menandakan akhir string.

**_7_**. `clear`

#### Kode

```c
void clear(byte* buf, unsigned int size) {
    unsigned int i;
    for (i = 0; i < size; i++) {
        buf[i] = 0x00;
    }
}
```

- loop i -> loop i dari index 0 sampai kurang dari size.
- buf[i] = 0x00; -> Setiap byte di buf menjadi 0x00.

---

### _c. Implementasikan perintah `echo`_

Perintah ini mengambil argumen yang diberikan (karakter keyboard) untuk perintah echo dan mencetaknya ke shell.

#### Kode Fungsi

```c
void EchoFunction(char* Input, char* buf) {
    int r = 5;
    while (Input[r] == ' ') {
        r++;
    }
    strcpy(Input + r, buf);
}
```

- r = 5 -> Inisialisasikan r dengan 5 (huruf e sampai spasi).
- r++ -> Skip spasi setelah 'echo'.
- strcpy(Input + r, buf); -> Salin isi buf ke Input + r (setelah 'echo').

#### Implementasi di main

```c
if (strcmp(cmd, "echo"))
    {
        char temp[128];
        EchoFunction(buf, temp);
        printString(temp);
        printString("\n");
    }
```

- if (strcmp(cmd, "echo")) -> Jika cmd sama dengan "echo".
- EchoFunction(buf, temp); -> Jalankan fungsi EchoFunction dengan argumen buf dan temp.
- printString(temp); -> Cetak string temp ke shell.

---

### _d. Implementasikan perintah `grep`_

Perintah ini mencari baris yang cocok dengan pola dalam inputnya dan mencetak baris yang cocok. grep hanya akan mengambil satu argumen menggunakan piping (|) dari perintah echo. Output harus berupa bagian dari argumen yang di-pipe yang diteruskan ke grep. Jika argumen tidak cocok, mengembalikan NULL

#### Kode

```c
void grepFunction(char* buf, char* pattern) {
    int lenbuf = strlen(buf);
    int lenpat = strlen(pattern);
    int found = 0;
    int c, d;
    int i;

    for (c = 0; c <= (lenbuf - lenpat); c++) {
        int match = 1;
        for (d = 0; d < lenpat; d++) {
            if (buf[c + d] != pattern[d]) {
                match = 0;
                break;
            }
        }

        if (match) {
            found = 1;
            for (i = 0; i < lenpat; i++) {
                buf[i] = pattern[i];  // copy pattern dari pattern ke buf, hanya bagian match
            }
            buf[lenpat] = '\0';

            printString(buf);
            printString("\n");
            return;
        }
    }

    // kalau tidak ditemukan
    buf[0] = 'N';
    buf[1] = 'U';
    buf[2] = 'L';
    buf[3] = 'L';
    buf[4] = '\0';

    printString("NULL\n");
}
```

- loop c -> mencari posisi awal dari pattern di string dari index 0 sampai lenbuf - lenpat.
- loop d -> membandingkan karakter demi karakter antara string dan pattern.
- jika match, copy pattern ke string dan cetak string tersebut.
- jika tidak match, cetak NULL.

---

### _e. Implementasikan perintah `wc`_

Perintah ini menghitung baris, kata, dan karakter dalam inputnya. wc tidak memerlukan argumen karena mendapat input dari pipe (|) dari perintah sebelumnya. Output harus berupa hitungan akhir dari argumen yang di-pipe yang diteruskan ke wc. Jika argumen tidak cocok, mengembalikan NULL atau 0

#### Kode

```c
void wcFuntion(char* buf) {
    int i;
    int len = 0;
    int kata = 0;
    int start = -1;
    int end = -1;

    for (i = 0; buf[i] != '\0'; i++) {
        if (buf[i] != ' ') {
            if (start == -1) {
                start = i;
            }
            end = i;

            if (i == 0 || buf[i - 1] == ' ') {
                kata++;
            }
        }
    }

    if (start != -1 && end != -1) {
        len = end - start + 1;
    }

    if (len == 0) {
        printString("0 Karakter\n");
    } else {
        char s[8];
        int x = len;
        int index = 0;

        while (x > 0) {
            s[index] = mod(x, 10) + '0';
            x = div(x, 10);
            index++;
        }

        for (i = index - 1; i >= 0; i--) {
            char ch[2];
            ch[0] = s[i];
            ch[1] = '\0';
            printString(ch);
        }
        printString(" Karakter\n");
    }

    if (kata == 0) {
        printString("0 Kata\n");
    } else {
        char s[8];
        int x = kata;
        int index = 0;

        while (x > 0) {
            s[index] = mod(x, 10) + '0';
            x = div(x, 10);
            index++;
        }

        for (i = index - 1; i >= 0; i--) {
            char ch[2];
            ch[0] = s[i];
            ch[1] = '\0';
            printString(ch);
        }
        printString(" Kata\n");
    }
    printString("1 Baris\n");
}
```

- for (i = 0; buf[i] != '\0'; i++) -> Menelusuri seluruh isi string buf.

- if (buf[i] != ' ') -> Jika karakter bukan spasi, artinya bagian dari kata atau isi.

- if (start == -1) start = i; -> Tandai awal isi teks pertama kali ditemukan.

- end = i; -> Tandai indeks terakhir dari isi teks.

- if (i == 0 || buf[i - 1] == ' ') kata++; -> Setiap kali huruf muncul setelah spasi, berarti itu awal kata baru → hitung kata++.

- if (start != -1 && end != -1) {len = end - start + 1;} -> Spasi di awal dan akhir tidak dihitung.

- Jika ada len dan kata ada -> len dan kata dikonversi ke string menggunakan fungsi `mod` dan `div` lalu diprint satu2.

- printString("1 Baris\n"); -> Print 1 baris karena input selalu hanya bisa 1 baris.

### Implementasi Fungsi `grepFunction` dan `wcFunction`.

**_1._** Pisahkan Pipe

```c
int pisahCMD(char* input, char cmds[][128]) {
    int count = 0;
    int i = 0, j = 0;

    while (1) {
        if (input[i] == '|' || input[i] == '\0')
        {
            cmds[count][j] = '\0'; //end string
            count++;
            j = 0;
            if (input[i] == '\0') break;
            i++;
            while (input[i] == ' ') i++; //skip space
        } else {
            cmds[count][j++] = input[i++];
        }
    }
    return count;
}
```

- cmds[count][j] = `\0` -> Menandai akhir string lalu count di-increment.
- cmds[count][j++] = input[i++]; -> Menambahkan karakter ke array cmds.
- return count -> Return jumlah komponen yang dipisahkan Pipe

**_2._** Gunakan fungsi berikut untuk eksekusi command pertama dan kedua

```c
void SecondCMD(char* Input) {
    char cmds[4][128];
    char buf[128];
    int i;
    int count;

    count = pisahCMD(Input, cmds);

    for (i = 0; i < count; i++)
    {
        char cmd[16];
        char isi[128];
        int j = 0;
        int k = 0;


        while (cmds[i][j] != ' ' && cmds[i][j] != '\0') {
            cmd[k++] = cmds[i][j++];
        }
        cmd[k] = '\0';

        while (cmds[i][j] == ' ') {
            j++;
        }
        strcpy(cmds[i] + j, isi);

        if (strcmp(cmd, "echo")) {
            int r = 0;
            while (isi[r] == ' '){
                r++;
            }
            strcpy(isi + r, buf);
        }
        else if (strcmp(cmd, "grep")) {
            grepFunction(buf, isi);
        } else if (strcmp(cmd, "wc")) {
            wcFuntion(buf);
        } else {
            printString("Command tidak diketahui\n");
            break;
        }
    }
}
```

- count = Jumlah command terpisah yang sudah difilter menggunakan `pisahCMD`.

- while (cmds[i][j] != ' ' && cmds[i][j] != '\0') {cmd[k++] = cmds[i][j++];} cmd[k] = '\0'; -> Mengambil command dari string yang sudah terpisah.

- while (cmds[i][j] == ' ') {j++;} strcpy(cmds[i] + j, isi); -> Mengambil isi dari string yang sudah terpisah.

- Gunakan fungsi yang sudah dibuat untuk mengolah command yang diinputkan menggunakan `strcmp`.

**_3._** Implementasi di main

```c
int main() {
    char buf[128];
    int i;

    clearScreen();
    printString("LilHabOS - B11\n");

    while (true) {
        printString("$> ");
        readString(buf);
        printString("\n");

        if (strlen(buf) > 0) {
            int isPipe = 0;
            for (i = 0; buf[i] != '\0'; i++) //cek apakah pipe ato ga
            {
                if (buf[i] == '|')
                {
                    isPipe = 1;
                }
            }

            if (isPipe) { //kalo pipe
                SecondCMD(buf);
            } else { //kalo ga pipe
                char cmd[128];
                int j = 0;

                while (buf[j] != ' ' && buf[j] != '\0') {
                    cmd[j] = buf[j];
                    j++;
                }
                cmd[j] = '\0';

                if (strcmp(cmd, "echo"))
                {
                    char temp[128];
                    EchoFunction(buf, temp);
                    printString(temp);
                    printString("\n");
                } else if (strcmp(cmd, "clear"))
                {
                    clearScreen();
                } else {
                    printString("Command tidak dipahami\n");
                }
            }
        }
    }
}
```

- char cmd[128];
  int j = 0;
  while (buf[j] != ' ' && buf[j] != '\0') {
  cmd[j] = buf[j];
  j++;
  }
  cmd[j] = '\0';

  -> Mengambil kalimat pertama dari input, lalu akan diteruskan ke permisalan yang akan mengeksekusi fungsi.

---

### _f. Buat otomatisasi untuk mengompilasi dengan melengkapi file makefile._

Untuk mengompilasi program, perintah make build akan digunakan. Semua hasil program yang dikompilasi akan disimpan di direktori bin/. Untuk menjalankan program, perintah make run akan digunakan.

#### Kode

```makefile
prepare:
	mkdir -p bin

bootloader:
	nasm -f bin src/bootloader.asm -o bin/bootloader

stdlib:
	bcc -ansi -c -Iinclude src/std_lib.c -o bin/std_lib.o

kernel:
	bcc -ansi -c -Iinclude src/kernel.c -o bin/kernel.o
	nasm -f as86 src/kernel.asm -o bin/kernel_asm.o

link:
	ld86 -o bin/kernel-load -d bin/kernel.o bin/std_lib.o bin/kernel_asm.o
	dd if=bin/bootloader of=bin/floppy.img bs=512 count=1 conv=notrunc
	dd if=bin/kernel-load of=bin/floppy.img bs=512 seek=1 conv=notrunc

build: prepare bootloader stdlib kernel link

run:
	bochs -f bochsrc.txt

```

#### Referensi Perintah

- dd
  -> dd if=<input_file> of=<output_file> bs=<block_size> count=<block_count> seek=<seek_count> conv=<conversion>

- nasm
  -> nasm -f <format> <input_file> -o <output_file>

- bcc
  -> bcc -ansi -c <input_file> -o <output_file>

- ld86
  -> ld86 -o <output_file> -d [file1.o file2.o ...]

#### Penjelasan Kode

- prepare -> Membuat direktori bin jika belum ada.

- bootloader -> Mengompilasi bootloader.asm menjadi
  bootloader.bin di direktori bin/ menggunakan nasm.

- stdlib -> Mengompilasi std_lib.c menjadi std_lib.o di direktori bin/ menggunakan bcc.

- kernel -> Mengompilasi kernel.c menjadi kernel.o di direktori bin/ menggunakan bcc dan mengompilasi kernel.asm menjadi kernel_asm.o di direktori bin/ menggunakan nasm.

- link -> Menggabungkan bootloader.bin, kernel.o, kernel_asm.o, dan std_lib.o menjadi floppy.img menggunakan Id86.

- build -> Menjalankan perintah prepare, bootloader, stdlib, kernel, dan link.
- run -> Menjalankan Bochs dengan k
  onfigurasi bochsrc.txt.


#### Output
<img width="802" alt="Screenshot 2025-06-22 at 18 44 43" src="https://github.com/user-attachments/assets/d489d716-99f6-42c9-8217-1af165df1001" />
<img width="386" alt="Screenshot 2025-06-22 at 18 46 03" src="https://github.com/user-attachments/assets/1462472d-1a49-4057-802e-21298cc04699" />






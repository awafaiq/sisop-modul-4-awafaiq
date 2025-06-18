#include "std_lib.h"
#include "kernel.h"

void printString(char* str);
void readString(char* buf);
void clearScreen();
void EchoFunction(char* Input);
void SecondCMD(char* Input);

int main() {
    char buf[128];
    int i;

    clearScreen();
    printString("LilHabOS - B11\n");

    while (true) {
        printString("$> ");
        readString(buf);
        printString("\n");

        if (strcmp(buf, "clear")) {
            clearScreen();
        }

        if (strlen(buf) > 0) {
            int isPipe = 0;
            for (i = 0; buf[i] != '\0'; i++) {
                if (buf[i] == '|') {
                    isPipe = 1;
                    break;
                }
            }

            if (isPipe) {
                SecondCMD(buf);
            } else {
                char cmd[128];
                int j = 0;

                while (buf[j] != ' ' && buf[j] != '\0') {
                    cmd[j] = buf[j];
                    j++;
                }
                cmd[j] = '\0';

                if (strcmp(cmd, "echo")) {
                    EchoFunction(buf);
                } else {
                    printString("Command tidak dipahami\n");
                }
            }
        }
    }
}

void printString(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        interrupt(0x10, 0x0E00 + str[i], 0, 0, 0);
        i++;
    }
}

void readString(char* buf) {
    int i = 0;
    char c;

    while (1) {
        c = interrupt(0x16, 0x0000, 0, 0, 0) & 0xFF;

        if (c == 13) {
            buf[i] = '\0';
            interrupt(0x10, 0x0E0D, 0, 0, 0);
            interrupt(0x10, 0x0E0A, 0, 0, 0);
            break;
        } else if (c == 8) {
            if (i > 0) {
                i--;
                interrupt(0x10, 0x0E08, 0, 0, 0);
                interrupt(0x10, 0x0E20, 0, 0, 0);
                interrupt(0x10, 0x0E08, 0, 0, 0);
            }
        } else {
            buf[i] = c;
            interrupt(0x10, 0x0E00 + c, 0, 0, 0);
            i++;
        }
    }
}

void clearScreen() {
    interrupt(0x10, 0x0600, 0x0700, 0x0000, 0x184F);
    interrupt(0x10, 0x0200, 0, 0, 0);
}

void EchoFunction(char* Input) {
    char buf[128];
    strcpy(Input + 5, buf);
    printString(buf);
    printString("\n");
}

void SecondCMD(char* Input) {
    char bef[128];
    char aft[128];
    int i = 0, j = 0;

    while (Input[i] != '|' && Input[i] != '\0') {
        bef[i] = Input[i];
        i++;
    }
    bef[i] = '\0';

    if (Input[i] == '|') {
        i++;
        while (Input[i] == ' ') i++;
        while (Input[i] != '\0') {
            aft[j++] = Input[i++];
        }
    }
    aft[j] = '\0';

    char buf[128];
    strcpy(bef + 5, buf);

    char grep[16];
    char grepbuf[128];
    i = 0;

    while (aft[i] != ' ' && aft[i] != '\0') {
        grep[i] = aft[i];
        i++;
    }
    grep[i] = '\0';

    if (strcmp(grep, "grep")) {
        int k = 0;
        if (aft[i] == ' ') i++;
        while (aft[i] != '\0') {
            grepbuf[k++] = aft[i++];
        }
        grepbuf[k] = '\0';

        int found = 0;
        int lenbuf = strlen(buf);
        int lenrepbuf = strlen(grepbuf);
        int m, n;

        for (m = 0; m <= (lenbuf - lenrepbuf); m++) {
            int match = 1;
            for (n = 0; n < lenrepbuf; n++) {
                if (buf[m + n] != grepbuf[n]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                found = 1;
                break;
            }
        }

        if (found) {
            printString(grepbuf);
            printString("\n");
        } else {
            printString("NULL\n");
        }
    } else if (strcmp(grep, "wc")) {
        int len = strlen(buf);
        int kata = 0;
        i = 0;

        for (i = 0; i < len; i++) {
            if ((i == 0 && buf[i] != ' ') || (buf[i] != ' ' && buf[i - 1] == ' ')) {
                kata++;
            }
        }

        if (len == 0) {
            printString("0 Karakter\n");
        } else {
            char s[4];
            int x = len;
            int index = 0;

            while (x > 0) {
                s[index++] = mod(x, 10) + '0';
                x = div(x, 10);
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
                s[index++] = mod(x, 10) + '0';
                x = div(x, 10);
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
    } else {
        printString("NULL \n");
    }
}



void SecondCMD(char* Input) {
    char bef[128];
    char aft[128];
    char buf[128];
    char grep[16];
    char grepbuf[128];
    int i = 0, j = 0, r = 0;
    int c = 0, d = 0;
    int lenbuf = 0;
    int lenrepbuf = 0;
    int found = 0;
    int match = 0;

    // Pisahkan sebelum dan sesudah '|'
    while (Input[i] != '|' && Input[i] != '\0') {
        bef[i] = Input[i];
        i++;
    }
    bef[i] = '\0';

    if (Input[i] == '|') {
        i++; // skip '|'
        while (Input[i] == ' ') i++; // skip spasi setelah pipe
        while (Input[i] != '\0') {
            aft[j++] = Input[i++];
        }
    }
    aft[j] = '\0';

    // Ambil argumen echo
    r = 5;
    while (bef[r] == ' ') r++;  // lewati spasi setelah 'echo'
    strcpy(buf, bef + r);       // <- arah dibetulkan

    // Cek command kedua (grep / wc)
    i = 0;
    while (aft[i] != ' ' && aft[i] != '\0') {
        grep[i] = aft[i];
        i++;
    }
    grep[i] = '\0';

    if (strcmp(grep, "grep")) {
        int k = 0;
        if (aft[i] == ' ') i++; // skip spasi
        while (aft[i] != '\0') {
            grepbuf[k++] = aft[i++];
        }
        grepbuf[k] = '\0';

        found = 0;
        lenbuf = strlen(buf);
        lenrepbuf = strlen(grepbuf);

        for (c = 0; c <= (lenbuf - lenrepbuf); c++) {
            match = 1;
            for (d = 0; d < lenrepbuf; d++) {
                if (buf[c + d] != grepbuf[d]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                found = 1;
                break;
            }
        }

        if (found) {
            printString(grepbuf);
            printString("\n");
        } else {
            printString("NULL\n");
        }

    } else if (strcmp(grep, "wc")) {
        int len = strlen(buf);
        int kata = 0;

        for (i = 0; i < len; i++) {
            if ((i == 0 && buf[i] != ' ') || (buf[i] != ' ' && buf[i - 1] == ' ')) {
                kata++;
            }
        }

        // Karakter
        if (len == 0) {
            printString("0 Karakter\n");
        } else {
            char s[4];
            int x = len;
            int index = 0;
            while (x > 0) {
                s[index++] = mod(x, 10) + '0';
                x = div(x, 10);
            }
            for (i = index - 1; i >= 0; i--) {
                char ch[2] = { s[i], '\0' };
                printString(ch);
            }
            printString(" Karakter\n");
        }

        // Kata
        if (kata == 0) {
            printString("0 Kata\n");
        } else {
            char s[8];
            int x = kata;
            int index = 0;
            while (x > 0) {
                s[index++] = mod(x, 10) + '0';
                x = div(x, 10);
            }
            for (i = index - 1; i >= 0; i--) {
                char ch[2] = { s[i], '\0' };
                printString(ch);
            }
            printString(" Kata\n");
        }

        printString("1 Baris\n");

    } else {
        printString("NULL \n");
    }
}


/*------------------------------------------------------------------------------------------------------------------*/
#include "std_lib.h"
#include "kernel.h"

// Insert global variables and function prototypes here
void printString(char* str);
void readString(char* buf);
void clearScreen();
void EchoFunction(char* Input);
void SecondCMD(char* Input);

int main() {
    char buf[128];
    int i;

    clearScreen();
    printString("LilHabOS - B11\n");

    while (true) {
        printString("$> ");
        readString(buf);
        printString("\n");

        if (strcmp(buf, "clear")) 
        {
            clearScreen();
        }    

        if (strlen(buf) > 0) {
            int isPipe = 0;
            for (i = 0; buf[i] != '\0'; i++) //cek apakah pipe ato ga
            {
                if (buf[i] == '|')
                {
                    isPipe = 1;
                    break;
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
                    EchoFunction(buf);
                } else {
                    printString("Command tidak dipahami\n");
                }
            }
        }
    }
}

void printString(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        interrupt(0x10, 0x0E00 + str[i], 0, 0, 0);
        i++;
    }
}

void readString(char* buf) {
    int i = 0;
    char c;

    while (1) {
        c = interrupt(0x16, 0x0000, 0, 0, 0) & 0xFF;

        if (c == 13) { // ENTER
            buf[i] = '\0';
            interrupt(0x10, 0x0E0D, 0, 0, 0); // CR
            interrupt(0x10, 0x0E0A, 0, 0, 0); // LF
            break;
        } else if (c == 8) { // BACKSPACE
            if (i > 0) {
                i--;
                // Move cursor back, erase character, move back again
                interrupt(0x10, 0x0E08, 0, 0, 0); // Backspace
                interrupt(0x10, 0x0E20, 0, 0, 0); // Space (overwrite)
                interrupt(0x10, 0x0E08, 0, 0, 0); // Backspace again
            }
        } else {
            buf[i] = c;
            interrupt(0x10, 0x0E00 + c, 0, 0, 0); // Echo input
            i++;
        }
    }
}

void clearScreen() {
    // Scroll entire screen up (effectively clears)
    // AH=0x06, AL=0 (scroll lines), BH=0x07 (white on black), CX=0, DX=0x184F (bottom right)
    interrupt(0x10, 0x0600, 0x0700, 0x0000, 0x184F);
    
    // Move cursor to top left
    // AH=0x02, BH=0 (page), DH=0 (row), DL=0 (col)
    interrupt(0x10, 0x0200, 0, 0, 0);
}

void EchoFunction(char* Input) {
    char buf[128];
    // strcpy(Input + 5, buf); //skip kata "echo", lalu selanjutnya disalin
    int r = 5;
    while (Input[r] == ' ') r++;  // lewati spasi setelah 'echo'
    strcpy(Input + r, buf);
    printString(buf);
    printString("\n");
}

void SecondCMD(char* Input) {
    char bef[128];
    char aft[128];
    char buf[128];       // Tambahkan
    char grep[16];       // Tambahkan
    char grepbuf[128];   // Tambahkan

    int i = 0, j = 0;
    int r = 0;           // Tambahkan
    int c = 0, d = 0;    // Tambahkan
    int lenbuf = 0;      // Tambahkan
    int lenrepbuf = 0;   // Tambahkan
    int found = 0;       // Tambahkan
    int match = 0;    
    // char bef[128];
    // char aft[128];
    // int i = 0, j = 0;

    // Pisahkan sebelum dan sesudah '|'
    while (Input[i] != '|' && Input[i] != '\0') {
        bef[i] = Input[i];
        i++;
    }
    bef[i] = '\0';

    if (Input[i] == '|') {
        i++; // skip '|'
        while (Input[i] == ' ') i++; // skip spasi setelah pipe
        while (Input[i] != '\0') {
            aft[j++] = Input[i++];
        }
    }
    aft[j] = '\0';

    // Ambil argumen echo
    // strcpy(bef + 5, buf); // lewati "echo"
    r = 5;
    while (Input[r] == ' ') r++;  // lewati spasi setelah 'echo'
    strcpy(Input + r, buf);

    // Cek command kedua
    // char grep[16];
    // char grepbuf[128];
    i = 0;

    while (aft[i] != ' ' && aft[i] != '\0') { //ambil kata setelah pipe
        grep[i] = aft[i];
        i++;
    }
    grep[i] = '\0';

    if (strcmp(grep, "grep")) {
        // Ambil pola grep
        int k = 0;
        if (aft[i] == ' ') i++; // skip spasi
        while (aft[i] != '\0') {
            grepbuf[k++] = aft[i++];
        }
        grepbuf[k] = '\0';

        // Pencocokan manual (tanpa strstr, hanya fungsi std_lib.h)
        found = 0;
        lenbuf = strlen(buf);
        lenrepbuf = strlen(grepbuf);

        for (c = 0; c <= (lenbuf - lenrepbuf); c++) {
            int match = 1;
            for (d = 0; d < lenrepbuf; d++) {
                if (buf[c + d] != grepbuf[d]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                found = 1;
                break;
            }
        }

        if (found) {
            printString(grepbuf);
            printString("\n");
        } else {
            printString("NULL\n");
        }
    } else if (strcmp(grep, "wc")) {
        int len = strlen(buf);
        int kata = 0;
        i = 0;

        //hitung kata
        for (i = 0; i < len; i++)
        {
            if ((i == 0 && buf[i] != ' ') || (buf[i] != ' ' && buf[i - 1] == ' ')) {
                kata++;
            }
        }

        if (len == 0) 
        {
            printString("0 Karakter\n");
        } else {
            char s[4];
            int x = len;
            int index = 0;

            while (x > 0)
            {
                s[index++] = mod(x, 10) + '0';
                x = div(x, 10);
            }
            
            for (i = index - 1; i >= 0; i--)
            {
                char ch[2];
                ch[0] = s[i];
                ch[1] = '\0';
                printString(ch);
            }
            printString(" Karakter\n");
        }
        
        if (kata == 0)
        {
            printString("0 Kata\n");
        } else {
            char s[8];
            int x = kata;
            int index = 0;

            while (x > 0)
            {
                s[index++] = mod(x, 10) + '0';
                x = div(x, 10);
            }

            for (i = index - 1; i >= 0; i--)
            {
                char ch[2];
                ch[0] = s[i];
                ch[1] = '\0';
                printString(ch);
            }
            
            printString(" Kata\n");
        }
        printString("1 Baris\n");
    } else {
        printString("NULL \n");
    }
}

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

            // Cek apakah input mengandung '|'
            for (i = 0; buf[i] != '\0'; i++) {
                if (buf[i] == '|') {
                    isPipe = 1;
                    break;
                }
            }

            if (isPipe) {
                SecondCMD(buf);
            } else {
                char cmd[128];
                int j = 0;

                // Ambil kata pertama sebagai command
                while (buf[j] != ' ' && buf[j] != '\0') {
                    cmd[j] = buf[j];
                    j++;
                }
                cmd[j] = '\0';

                if (strcmp(cmd, "echo")) {
                    EchoFunction(buf);
                } else if (strcmp(cmd, "clear")) {
                    clearScreen();
                } else {
                    printString("Command tidak dipahami\n");
                }
            }
        }
    }
}

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
                    EchoFunction(buf);
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

void clearScreen() {
    interrupt(0x10, 0x0600, 0x0700, 0x0000, 0x184F);
    interrupt(0x10, 0x0200, 0, 0, 0);
}

void EchoFunction(char* Input) {
    char buf[128];
    int r = 5;
    while (Input[r] == ' ') r++;  //lewati spasi setelah 'echo'
    strcpy(Input + r, buf);
    printString(buf);
    printString("\n");
}

void SecondCMD(char* Input) {
    char bef[128];
    char aft[128];
    char buf[128];
    char grep[16];
    char grepbuf[128];
    int i = 0, j = 0;
    int r = 0;
    int c = 0, d = 0;
    int lenbuf = 0;
    int lenrepbuf = 0;
    int found = 0;
    int match = 0;
    int len = 0;
    int kata = 0;
    int start = 0;
    int end = 0;

    //pisahkan sebelum dan sesudah '|'
    while (Input[i] != '|' && Input[i] != '\0') {
        bef[i] = Input[i];
        i++;
    }
    bef[i] = '\0';

    if (Input[i] == '|') {
        i++;
        while (Input[i] == ' ') i++;
        while (Input[i] != '\0') {
            aft[j] = Input[i];
            j++;
            i++;
        }
    }
    aft[j] = '\0';

    //ambil echo
    r = 5;
    while (bef[r] == ' ') r++;
    strcpy(bef + r, buf);
    
    //ambil cmd setelah pipe
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
            grepbuf[k] = aft[i];
            i++;
            k++;
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
            strcpy(buf, grepbuf);
        } else {
            printString("NULL\n");
        }
    } else if (strcmp(grep, "wc")) {
        r = 5;
        while (bef[r] == ' ') {
            r++; //skip spasi abis echo
        }

        len = 0;
        kata = 0;
        start = -1;
        end = -1;

        for (i = r; (bef[i] != '\0') && (bef[i] != '|'); i++) {
            if (bef[i] != ' ') {
                if (start == -1) start = i;
                end = i;

                if ((i == r) || (bef[i - 1] == ' ')) {
                    kata++;
                }
            }
        }

        if (start != -1 && end != -1) {
            len = end - start + 1;
        } else {
            len = 0;
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

            for (i = index - 1; i >= 0; i--)
            {
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

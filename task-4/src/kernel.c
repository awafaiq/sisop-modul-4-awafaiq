#include "std_lib.h"
#include "kernel.h"

// Insert global variables and function prototypes here
void printString(char* str);
void readString(char* buf);
void clearScreen();
void EchoFunction(char* Input, char* buf);
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

void EchoFunction(char* Input, char* buf) {
    int r = 5;
    while (Input[r] == ' ') {
        r++; //lewati spasi setelah 'echo'
    }  
    strcpy(Input + r, buf);
}

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
                buf[i] = pattern[i];  // copy pattern dari buf ke buf, hanya bagian match
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

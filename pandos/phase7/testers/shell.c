
/*
 * shell_interpreter.c
 * Author: Haruka Yamamoto
 * Date: 15/10/2024
 * 
 * Description:
 * This file implements a simple shell-like command interpreter that allows 
 * users to perform various actions through a command-line interface. The 
 * interpreter accepts commands for printing text, retrieving the current 
 * time, displaying help information, showing ASCII art galleries, and 
 * performing basic arithmetic calculations. It operates within an infinite 
 * loop, continuously prompting the user for input until an exit command 
 * is issued.
 *
 * Commands Supported:
 * 1. EXIT: Terminates the program and exits the shell.
 * 2. PRINT: Prompts the user for input and prints the text to the terminal.
 * 3. TIME: Displays how long the shell has been running in seconds.
 * 4. HELP: Provides a list of available commands and their descriptions.
 * 5. GALLERY: Cycles through ASCII art galleries to display various artworks.
 * 6. CALCULATE: Allows the user to perform basic arithmetic operations 
 *    (addition, subtraction, multiplication, division) on two integers, 
 *    including support for negative values.
 *
 * Includes:
 * - localLibumps.h: Contains declarations for system calls used in the program.
 * - tconst.h: Defines constant values used throughout the code.
 * - print.h: Provides declarations for print-related functions.
 *
 * Constants:
 * - EXIT: Command identifier for exiting the shell.
 * - PRINT: Command identifier for the print function.
 * - TOD: Command identifier for the time of day function.
 * - HELP: Command identifier for the help function.
 * - GALLERY: Command identifier for displaying ASCII art galleries.
 * - CALCULATE: Command identifier for the calculator function.
 *
 * Function Prototypes:
 * - void exit(): Terminates the program.
 * - void printing(): Prompts the user for input and prints it to the terminal.
 * - void time(): Retrieves and displays the current running time of the shell.
 * - void help(): Displays a list of available commands and their descriptions.
 * - void gallery(): Cycles through different ASCII art displays.
 * - void error(): Displays an error message when an invalid operation occurs.
 * - void intToStr(int num, char *str): Converts an integer to its string representation.
 * - int strcmp(const char* s1, const char* s2): Compares two strings and returns an integer 
 *   indicating their lexicographical order.
 * - void calculator(): Parses user input for arithmetic calculations and performs 
 *   the specified operation.
 * - void command(int commandNum): Executes the command associated with the given command number.
 * - int getCase(const char* input): Maps user input to command identifiers.
 *
 * Main Functionality:
 * The main function runs an infinite loop, repeatedly prompting the user for commands 
 * and executing the corresponding functions based on user input. The loop continues until 
 * the user issues an exit command.
 *
 */



#include "h/localLibumps.h"  /* Include local library for system calls */
#include "h/tconst.h"        /* Include constant definitions */
#include "h/print.h"         /* Include print function declarations */

#define EXIT 1              /* Command identifier for exit */
#define PRINT 2             /* Command identifier for print */
#define TOD 3               /* Command identifier for time of day */
#define HELP 4              /* Command identifier for help */
#define GALLERY 5           /* Command identifier for gallery */
#define CALCULATE 6        /* Command identifier for calculator */

void miyajima();           /* Function prototypes */
void straydog();
void nz();

/* Function to terminate the program */
void exit() {
    print(WRITETERMINAL, "exit\n"); /* Print exit message */
    SYSCALL(TERMINATE, 0, 0, 0); /* Terminate the program */
}

/* Function to print user input */
void printing() {
    print(WRITETERMINAL, "Type what you want to print: "); /* Prompt user */
    char buf[15]; /* Buffer for user input */
    
    /* Read user input from terminal */
    int status = SYSCALL(READTERMINAL, (int)&buf[0], 0, 0); 
    buf[status] = EOS; /* Null-terminate the input */
    
    print(WRITETERMINAL, "Printing...\n"); /* Indicate printing process */
    print(WRITETERMINAL, buf); /* Print user input */
    print(WRITETERMINAL, "\n"); /* New line */
}

/* Function to get the current time */
void time() {
    print(WRITETERMINAL, "Shell has been running for this many seconds:\n"); /* Print time prompt */
    
    unsigned int time = SYSCALL(GET_TOD, 0, 0, 0); /* Retrieve current time */
    time /= SECOND * 10; /* Convert time to appropriate unit */
    
    char str[30]; /* Buffer for time string */
    intToStr(time, str); /* Convert time to string */
    
    print(WRITETERMINAL, str); /* Print the time */
    print(WRITETERMINAL, "\n"); /* New line */
}

/* Function to display help information */
void help() {
    print(WRITETERMINAL, "Here are the shell commands:\n"); /* Print help header */
    print(WRITETERMINAL, "  EXIT: By typing 'exit' you exit the shell\n"); /* Exit command description */
    print(WRITETERMINAL, "  PRINT: By typing 'print' you can print to the terminal\n"); /* Print command description */
    print(WRITETERMINAL, "  TOD: By typing 'time' you can get how long the shell has been running for in seconds\n"); /* Time command description */
    print(WRITETERMINAL, "  HELP: By typing 'help' you get to this output\n"); /* Help command description */
    print(WRITETERMINAL, "  GALLERY: By typing 'gallery' you get to see some cool ASCII art photos\n"); /* Gallery command description */
    print(WRITETERMINAL, "  CALCULATOR: By typing 'calc' you can do basic math by -> num1 operand num2\n              Keep input numbers below 1000\n"); /* calc command description */
}

/* Function to handle gallery commands */
void gallery() {
    static int flag; /* Static variable to toggle between galleries */
    
    if (flag == 0) {
        flag = 1; /* Switch to first gallery */
        miyajima(); /* Show first ASCII art */
    } else if (flag == 1) {
        straydog(); /* Show second ASCII art */
        flag = 2; /* Reset flag */
    } else if (flag == 2) {
        nz();
        flag = 0;
    } else {
        flag = 0; /* Reset flag */
        error(); /* Call error function if in invalid state */
    }
}

/* Function to display an error message */
void error() {
    print(WRITETERMINAL, "An error occurred, try again\n"); /* Print error message */
}

/* Function to convert an integer to a string */
void intToStr(int num, char *str) {
    int i = 0; /* Index for string */
    int j;
    int isNegative = 0; /* Flag for negative numbers */

    if (num < 0) { /* Check if number is negative */
        isNegative = 1; /* Set negative flag */
        num = -num; /* Convert to positive */
    }

    do {
        str[i++] = (num % 10) + '0'; /* Extract digit and convert to char */
        num /= 10; /* Divide number by 10 */
    } while (num > 0); /* Continue until num is 0 */

    if (isNegative) { /* If negative, add '-' */
        str[i++] = '-';
    }
    
    str[i] = EOS; /* Null terminate the string */
    
    /* Reverse the string */
    for (j = 0; j < i / 2; j++) {
        char temp = str[j]; /* Swap characters */
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

/* Function to compare two strings */
int strcmp(const char* s1, const char* s2) {
    /* Compare characters until we find a mismatch or reach the end */
    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 < *s2) {
            return -1; /* s1 is less than s2 */
        } else if (*s1 > *s2) {
            return 1; /* s1 is greater than s2 */
        }
        s1++;
        s2++;
    }

    /* Check if one string is a prefix of the other */
    if (*s1 == '\0' && *s2 == '\0') {
        return 0; /* Strings are equal */
    } else if (*s1 == '\0') {
        return -1; /* s1 is shorter than s2 */
    } else {
        return 1; /* s2 is shorter than s1 */
    }
}


/* Function to handle basic arithmetic calculations */
void calculator() {
    print(WRITETERMINAL, "Enter calculation: ");
    char buf[50]; /* Buffer for user input */
    
    /* Read user input from terminal */
    int status = SYSCALL(READTERMINAL, (int)&buf[0], 0, 0);
    buf[status] = EOS; /* Null-terminate the input */

    int num1 = 0, num2 = 0; /* Initialize numbers */
    char operator; /* Variable for operator */
    char result[30]; /* Buffer for result */
    int i = 0;
    int isNegative1 = 0, isNegative2 = 0; /* Flags for negative numbers */

    /* Check for negative sign for the first number */
    if (buf[i] == '-') {
        isNegative1 = 1; /* Set negative flag */
        i++;
    }

    /* Parse the first number */
    while (buf[i] >= '0' && buf[i] <= '9') {
        num1 = num1 * 10 + (buf[i] - '0'); /* Convert char to int */
        i++;
    }
    
    if (isNegative1) {
        num1 = -num1; /* Make the number negative */
    }

    /* Skip any whitespace */
    while (buf[i] == ' ') {
        i++;
    }

    /* Get the operator */
    operator = buf[i++];
    
    /* Skip any whitespace */
    while (buf[i] == ' ') {
        i++;
    }

    /* Check for negative sign for the second number */
    if (buf[i] == '-') {
        isNegative2 = 1; /* Set negative flag */
        i++;
    }

    /* Parse the second number */
    while (buf[i] >= '0' && buf[i] <= '9') {
        num2 = num2 * 10 + (buf[i] - '0'); /* Convert char to int */
        i++;
    }

    if (isNegative2) {
        num2 = -num2; /* Make the number negative */
    }

    if( ((num1 < -1000) || ((num1 > 1000)) && ((num2 > 1000)) || (num2 < -1000))) {
        print(WRITETERMINAL, "Error: Too large.\n");
        return;
    }

    /* Perform the calculation based on the operator */
    int calcResult = 0; /* Variable for calculation result */
    if (operator == '+') {
        calcResult = num1 + num2;
    } else if (operator == '-') {
        calcResult = num1 - num2;
    } else if (operator == '*') {
        calcResult = num1 * num2;
    } else if (operator == '/') {
        if (num2 != 0) {
            calcResult = num1 / num2;
        } else {
            print(WRITETERMINAL, "Error: Division by zero.\n");
            return;
        }
    } else {
        print(WRITETERMINAL, "Error: Unknown operator.\n");
        return;
    }
    
    /* Convert result to string and print */
    intToStr(calcResult, result);
    print(WRITETERMINAL, "Result: ");
    print(WRITETERMINAL, result);
    print(WRITETERMINAL, "\n");
}



/* Function to handle different commands */
void command(int commandNum) {
    switch (commandNum) {
        case EXIT: /* Exit command */
            exit(); /* Call exit function */
            break;
        case PRINT: /* Print command */
            printing(); /* Call print function */
            break;
        case TOD: /* Time command */
            time(); /* Call time function */
            break;
        case HELP: /* Help command */
            help(); /* Call help function */
            break;
        case GALLERY: /* Gallery command */
            gallery(); /* Call gallery function */
            break;
        case CALCULATE: /* New case for calculator */
            calculator(); /* Call calculator function */
            break;
        default: /* Handle unknown commands */
            error(); /* Call error function */
            break;
    }
}

/* Function to get command case based on user input */
int getCase(const char* input) {
    int output = -1; /* Initialize output to -1 (unknown command) */
    
    /* Check input against known commands */
    if (strcmp(input, "exit") == 0) {

        output = EXIT; /* Set output to EXIT if input matches */

    } else if (strcmp(input, "print") == 0) {

        output = PRINT; /* Set output to PRINT */

    } else if (strcmp(input, "time") == 0) {

        output = TOD; /* Set output to TOD */

    } else if (strcmp(input, "help") == 0) {

        output = HELP; /* Set output to HELP */

    } else if (strcmp(input, "gallery") == 0) {

        output = GALLERY; /* Set output to GALLERY */

    } else if (strcmp(input, "calc") == 0) { /* Check for new calculator command */

        output = CALCULATE; /* Set output to CALCULATE */
    }

    return output; /* Return the command identifier */
}

/* Main function to run the shell */
int main() {
    /* Main loop to process user input */

    while (1) {
        char input[50]; /* Buffer for user input */

        print(WRITETERMINAL, "umps3> "); /* Prompt for command */
        
        /* Read command from terminal */

        int status = SYSCALL(READTERMINAL, (int)&input[0], 0, 0); 

        input[status] = EOS; /* Null-terminate the input string */

        /* Get the command number corresponding to user input */

        int commandNum = getCase(input); 

        command(commandNum); /* Execute the command */
    }

    return 0; /* Return success (never reached) */
}

void miyajima() {
                                                                                                                        
    /* Print ASCII art to the terminal */
    print(WRITETERMINAL, "\n                                                                           A                              \n"); 
    print(WRITETERMINAL, "                                                                         AA                               \n"); 
    print(WRITETERMINAL, "                                                                 SDFBAAHAJA                               \n"); 
    print(WRITETERMINAL, "                              EFDE YIn       ht   ACFEEDFCAAAAELNMKKKIHE                                  \n"); 
    print(WRITETERMINAL, "                                NLHGCAAAAABDAAAAAADFIMONOPPQQQQRQRRTQC                                    \n"); 
    print(WRITETERMINAL, "                                  SSSTTSTQRRQQOMADACHKMQJD WSO                                            \n"); 
    print(WRITETERMINAL, "                                        hKI     AAAFA      UUL                                            \n"); 
    print(WRITETERMINAL, "                                       4RoIS     WaANQV22UU rZ                                            \n"); 
    print(WRITETERMINAL, " hi                                      qX      WYXVXZ253m Z  jWk rwuxwttsrqsqswqtvtspn                  \n"); 
    print(WRITETERMINAL, " fddci jh                                eR       psppokmot mLmomkpn qsszkqpnrnsusrpspnmnpnosy      tvuss \n"); 
    print(WRITETERMINAL, " bcdccfggjifo                            aQ  imkkqmngbhmjgp UQmnsmmollppnnonnkospmrkhrpqmorjjkomjomoonlqq \n"); 
    print(WRITETERMINAL, " 966048969435c656896887889j    AAAAAADu  TRmb0988612ZTSRMV0 gYejBAAAAssiknroppoqo knnrrkopomnqjmojilkgjle \n"); 
    print(WRITETERMINAL, " AB0bd6412ZZ525Y6Z421668623527668aSeaa09 TP584ML5Z8715 SZ13   abb2PO8dcmegef4ijeca8279d50d99bd68acfgcbfcc \n"); 
    print(WRITETERMINAL, " AAZCTY43533WYYZ3Y2Y22Z12331X5X2nRRQQQLNOQONKCNNA4a90ePQMLPKJhOHKJMQc6ca969b0aba378898fh80b5fa0d0d90bfe9a \n"); 
    print(WRITETERMINAL, " FAAAAAMRWRRVSOTRQSSSTPSQQNSS5c6Y R75bppQQK lkOQY325ke5S0b0 MLN6 4OO8f6md4fQUKMQOU90gng         ic73587gd \n"); 
    print(WRITETERMINAL, " 4  nAAX x d931QQSQJLVTNR0 Pr gY3 U31m3  PMj91SMCMHFDL SKKM qMNXg RPLWGI n5bcNHPTSU  3 taDWNGSLDKZXjeYSL4 \n"); 
    print(WRITETERMINAL, "                           zxzyws 1ywpu aQMwxxUNwxv yx 9zx  hLNruqROourqplppqqqprprnnmospnnmmmnlkmkniiebj \n"); 
    print(WRITETERMINAL, "                                 hSJ    MKL  6GM       5X  zkIL  dKN                          z  wx     z \n"); 
    print(WRITETERMINAL, "                                mJOEBAAAIKLACACG     GMOIAAADRIAAACF                                   z  \n"); 
    print(WRITETERMINAL, " K                               iJi   qEEC   UA      RKA  nEEFyz JE        x       z   v o z    yw   f   \n"); 
    print(WRITETERMINAL, "                                 VAA9UOF AA  pAA       AA  FJAA8 xFAu ztyz            z         z    J  w \n"); 
    print(WRITETERMINAL, "                                xUAA  aATAA baAAnitxup5AAv 7AKAAlkIAmiq ffjqogu v                    A    \n"); 
    print(WRITETERMINAL, "                      z  z  vzw qZQdr tiUSUy sdb     i8KK  l5RLq  Z7                       s  z     yk iu \n"); 
    print(WRITETERMINAL, "                                 re    k14j  wwX      jkq  g7X8  qXQ                      pm  to6jqwuu    \n"); 
    print(WRITETERMINAL, "                                  nz   9llr   z       tvpvwsniuq   z                  zz   zuv9mtrwwtrmov \n"); 
    print(WRITETERMINAL, "                                 zu    yuzu           svvy qqcvyu d                zg  z       l p vkrteu \n"); 
    print(WRITETERMINAL, "                               zr r zyonlkw  ws   wxwkkbi uddimtsjzexlch x              x               w \n"); 
    print(WRITETERMINAL, "                                nngluptib2dkmnjsv o  q9kqujd5kezsnunh            t                        \n"); 
    print(WRITETERMINAL, "         w        z   z u  cuqv qpqkluyqsjmptriponnjrheksmgscdvuuzo p rz u  x   z           yk          z \n"); 
    print(WRITETERMINAL, "               y     xlq vwxz  ynisql rqilidqrlkofsz m6bjvj8j0vlornl rz       xt                          \n"); 
    print(WRITETERMINAL, "    v          r  w yy  nzsppxyyosjnlikummj5nlgiuynk 9hjinmnoc tecm ty          u         z p             \n");
}            

void straydog() {
                                                                                                                        
    /* Print ASCII art to the terminal */
    print(WRITETERMINAL, "\nAAAZ777pdpddTd7ZM77j77Z77TAAAAAAAAAA7ZZMZMZ7TZTTMGAAAAAAAAAAAAAAAAAAAAAAAAA                     pZ7ZTGGGAAAAAAAAAM\n"); 
    print(WRITETERMINAL, "AAAAAAAAAAAMGGMMTMMMGTdZTZGAAAAAAAAAG7w7TGZZTMTAMAAAAAAAAAAAAAAAAAAAAAAAAAA  pAGAAAGTwdGAAA7  TAAAAAAAAAGAAAAAAAAA\n"); 
    print(WRITETERMINAL, "AAAAAAAAAAAAGGGAGGMMMTGMGAAAAAAAAAAAAZTGG  ZMMGd   AAAGGGAGGAAAAAAAAAAAAAAA7       pdTM77dMG GGAAAAAAAAAAAAAAAAAAA\n"); 
    print(WRITETERMINAL, "AAAAAAAAAAAAAAAAAAAAAAAMAGAAAAAAT         GZAAAAAAAAAAAAAAAAAAAAAG                       Z7AAATGAAAAAAAAAAAAAAAAAA\n"); 
    print(WRITETERMINAL, "AAAAAAAAAAAAAAAAAAAAAAAAAAAA       GAAAw AAAAAAAAAAAAAAAA77                          G AAAAAjAAAAAAAAAAAAAAGAAAAAA\n"); 
    print(WRITETERMINAL, "AAAAAAAAAAAAAAAAAAAAAAAA      GdMAAAAAAAAAAAAAAAAAA A7TGAA                        j7 AAAAAAAAAAdpMGAAGAAAAAAAAAAAA\n"); 
    print(WRITETERMINAL, "AAAAGMZ7                  wA  AAAAAAAAAAAAAAAAAAAAAZd7TGZ                            pAAAAAAAAAAAGAAAAAAAAAAAAAAAA\n"); 
    print(WRITETERMINAL, "AAAT                     AAAAAAAAAAAAAAAAAAAAAAAAAMGAGT               Z          Z  j AGAAAAAAAAAAGAAMMGAGAAAGAAAA\n"); 
    print(WRITETERMINAL, "AAAMw                   AAAAAAAAAAAAAAAAAAAAAAAAAAAAjAw          Z   jjpMj jwj  jAAAAAdMMAAAAAAAAAAAGZdZTMGGGMMAAA\n"); 
    print(WRITETERMINAL, "AAAAw             7p jMGAAMp    AAAAAAAAAAAAAAAAGAA7Mw       p      AMAAdM      AAAAAAAA AAAAAAAAAAAA   jdZTZZTAAG\n"); 
    print(WRITETERMINAL, "AAAAAA7ZAAAA   T T   j AAA        A AAAAAAAAAAAAGp              7A   G77Td w  d  AAAAAAAAAAAAAAAAAAAAA   pjwZ77TGT\n"); 
    print(WRITETERMINAL, "AAAAGAAAAAA TAMAA       TA        AAAAAAAAAAAAAw                    ZA pAM  MAG   AAAAAAAAAAAAAAAAAAAA    w wddZTd\n"); 
    print(WRITETERMINAL, "ZTT7TAA7   wjjTAdpAAAAAAM   ZM    AAAAAAAAAAAAAA                  Z  ZAw    AAG    AAAAAAAAAAAAAAAAAAAA     wwjp77\n"); 
    print(WRITETERMINAL, "ddwjAA      pZ7GTGGAAAAAAAA MAAj wAAAAAAAAAAAAAA               A d             AZ7AAAAAAAAAAAAAAAAAAAAA   p pw wdd\n"); 
    print(WRITETERMINAL, "pd AA       ZMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA            A  d          AAAAAAAAAAAAAAAAAAAAAAAAAAAAA    jdpZT7\n"); 
    print(WRITETERMINAL, "  pppw       AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA          wAZ         AAAAAAAAAAAAAAAAAAAAAAAAAAAAA     jjdj\n"); 
    print(WRITETERMINAL, "  pw        dGAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAj7 dA GAA         AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA      djp\n"); 
    print(WRITETERMINAL, "             MAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA A7A    7 A   AAAAAAAAAAAAAAAAAAAAAAAAAAAAA     wjd \n"); 
    print(WRITETERMINAL, "             T7AAAAMMA  MAAMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA7AAAT  pZ    ZAAAAAAAAAAAAAAAAAAAAAAAAAAAA      pd \n"); 
    print(WRITETERMINAL, "     dp      7T AAAMGd7G7AAAMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA   jGG    AAAAAAAAAAAAAAAAAAAAAAAAAAAA      ww \n"); 
    print(WRITETERMINAL, "              w 7AAAAAMTAAAAGAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA   wA   AAAAAAAAAAAAAAAAAAAAAAAAAAAAA      d \n"); 
    print(WRITETERMINAL, "                 7AAAGAMGMAAZTAAAGAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA  AA   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAw       \n"); 
    print(WRITETERMINAL, "                  GAAAM7jTj7T      AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAM 7AA  AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA       \n"); 
    print(WRITETERMINAL, "                  GGAGGMdd          jAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA       AAAAAAAAAAAAAAAAAAAAAAAAAAAAAd      \n"); 
    print(WRITETERMINAL, "                 dGAAA77w            AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA w   7AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA      \n"); 
    print(WRITETERMINAL, "                 TGGj     w           MMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA     AAAAAAAAAAAAAZAAAAAAAAAAAAAAAA      \n"); 
    print(WRITETERMINAL, "                 djdZT    MM           MGAAAAAAAAA       ZAAAAAAAMAAj         ZAAMAAAAAAA    AAAAAAAAAAAAAAA      \n"); 
    print(WRITETERMINAL, "                AAAAG7  Gpj              AAAAAAAAA          AAAA              AAAAAAAAAA     AAAAAAAAAAAAAAA    p \n"); 
    print(WRITETERMINAL, "                       AG                 AAAAAAAA         TAAAA              AAAAAAAAAd    wZAGAAAAAAAAAAAAAAAAAA\n"); 
    print(WRITETERMINAL, "                                          GAAAAAAAG        AAAA               pAAAAAAAA       GAAAAAAAAAAAAAAj ppp\n"); 
    print(WRITETERMINAL, "                                           TAAAAAAA     MT AAAd                AAAAAAAA         AAAAAAAAAAAAA     \n"); 
    print(WRITETERMINAL, "                                           jTAAAAAA     TAAAAA                TAAAAAAAA          AAAAAAAAAAAAA    \n"); 
    print(WRITETERMINAL, "                                            GAAAAAAA    AAAAAA                jAAAAAAAA           TAAAAAAAAAAAT   \n"); 
    print(WRITETERMINAL, "                                             MAAAAAA   TAAAAAA                  AAAAAAAA           ZMAAAAAAAAA7   \n"); 
    print(WRITETERMINAL, "                                               AAAAA   AAAAAA                    7AAAAAAA            dAAAAAAAAA   \n"); 
    print(WRITETERMINAL, "                                               AAAA   GAAAAAA                     AAAAAAAA             wAAAAAAA   \n"); 
    print(WRITETERMINAL, "                                              MAAA     AAAAAAAAAAA7               TAAAAAAA              AAAAAAA   \n"); 
    print(WRITETERMINAL, "                                        MAAAAAAAAA   dGAAAAAAAAAAAAAAGGGAGMTTMTMTZAAAAAAAZd 7jpd7M7Gdjp7AAAAAAAATT\n"); 
    print(WRITETERMINAL, "                                       G AAAAAAAAAAAAAAAAAAG7                     AAAAAAA      dZd7777ZTAAAAAAAAGM\n"); 
    print(WRITETERMINAL, "                                       wAAAAAAAAAGMZ77TZpT7TTTTMAAAAMGMdjZZp      AAAAAAd               AAAAAAAAMG\n"); 
    print(WRITETERMINAL, "                                                             M        AGj7jp p  jwAAAAAA            djpdAAAAAAAATG\n"); 
    print(WRITETERMINAL, "                                                                                wAAAAAAAp   wjdwZTZj AAAAAAAAAAAZT\n"); 
    print(WRITETERMINAL, "                                                                               pAAAAAAAA     wAAAGjZAAAAAAAAAAAATZ\n"); 
    print(WRITETERMINAL, "                                                                              AAAAAAAAAA        jwjM AAAAAAAAAAAAA\n"); 
    print(WRITETERMINAL, "                                                                             MAAAAAAAAAAAAAATMZTjd7MMAAAAAAAAAAGTG\n"); 
    print(WRITETERMINAL, "                                                                               AAAAAAAAAAGMMTTTdj7777pjjj7dd7dw7M7\n"); 
}    

void nz() 
{
    print(WRITETERMINAL, "\n                                                                                                    \n");
    print(WRITETERMINAL, "                                                   vrmr                                             \n");
    print(WRITETERMINAL, "                                                     rrvv     v                                     \n");
    print(WRITETERMINAL, "                                                      vmvr                                          \n");
    print(WRITETERMINAL, "                                                      vvmmmmmmvv                                    \n");
    print(WRITETERMINAL, "                                                       rmmmmmmmvm                                   \n");
    print(WRITETERMINAL, "                                                        vrmmmmmmmmv                                 \n");
    print(WRITETERMINAL, "                                                          mmmmmmmmr                                 \n");
    print(WRITETERMINAL, "                                                           vmmmmmmv                                 \n");
    print(WRITETERMINAL, "                                                            vmmmmmmvv   vv                          \n");
    print(WRITETERMINAL, "                                                              mrvrimr    vr  v                      \n");
    print(WRITETERMINAL, "                                                               vr mmvv   vv                         \n");
    print(WRITETERMINAL, "                                                             v  vmmmv     mvv                       \n");
    print(WRITETERMINAL, "                                                                  mmrrvvvvmmrv                      \n");
    print(WRITETERMINAL, "                                                                  vrvrmmm vmmr                      \n");
    print(WRITETERMINAL, "                                                                  vvmimmmmmmmm                      \n");
    print(WRITETERMINAL, "                                                                    mmmmmmmmmmv                 v   \n");
    print(WRITETERMINAL, "                                                                    vmmmmmmmmmrvv         v  vmmmmv \n");
    print(WRITETERMINAL, "                                                                   vvmmmmmmmmmmmmmmrv      rmmmmmmr \n");
    print(WRITETERMINAL, "                                                                  v vmmmmmmmmmmmmmmmmimrmmmmmmmmmr  \n");
    print(WRITETERMINAL, "                                                                    mmmmmmmmmmmmmmmmmmmmmmmmmmmmmr  \n");
    print(WRITETERMINAL, "                                                                   vmmmmmmimmmmmmmmmmmmmmmmmmmmmmv  \n");
    print(WRITETERMINAL, "                                                                   rmmmmmmmmmmmmmmmmmmmmmimmmmmmr   \n");
    print(WRITETERMINAL, "                                                                   mmmmmmmmmmmmmmmmmmmmmmmmmmmv     \n");
    print(WRITETERMINAL, "                                                                vvmmmmmmmmmmmmmmmimmmmmmmmmmmm      \n");
    print(WRITETERMINAL, "                                                             rmmmmmmmmmmmmmmmmmmmmmmmmmrv    mv     \n");
    print(WRITETERMINAL, "                                                         v  vmmmmmmmmmmmmmimmmmmmmmmmmv v  v   v    \n");
    print(WRITETERMINAL, "                                                             vrmmmmmmmmmmmmmmmmmmmmmmm              \n");
    print(WRITETERMINAL, "                                                                 vmmmmmmmmmmmmmmmmmmmmmv            \n");
    print(WRITETERMINAL, "                                                                    vvmmmmmmmmmmmmmmmmv             \n");
    print(WRITETERMINAL, "                                                                       vmmmmmmmmmmmmim v            \n");
    print(WRITETERMINAL, "                                                                        rmmmmmmmmmmm                \n");
    print(WRITETERMINAL, "                                                 v vv                   mmmmmmmmmmmr                \n");
    print(WRITETERMINAL, "                                                 vir                   vmmmmmmmmmm                  \n");
    print(WRITETERMINAL, "                                               rmmimmrr     vr v     vvmmmmmmmmmr                   \n");
    print(WRITETERMINAL, "                                             v rmmmmimr v   rvrrv    vmmmmmmmmmr                    \n");
    print(WRITETERMINAL, "                                               mmmmmmmm vmmmmrrrrv vrmmmmmmmmmv  v                  \n");
    print(WRITETERMINAL, "                                              vmmmmmmmmiimmmmmm     vrrrmmmmr                       \n");
    print(WRITETERMINAL, "                                         v   rmmmmmmmmmmmmmmmmmr       vrrv                         \n");
    print(WRITETERMINAL, "                                          rrmmmimimmmmmmmmmmmmmmv                                   \n");
    print(WRITETERMINAL, "                                         vmmmmmmmmmmmmmmmmmmimi v                                   \n");
    print(WRITETERMINAL, "                                         mmmmmmmmmmmmmmmmmimmm                                      \n");
    print(WRITETERMINAL, "                                       vvmmmmmmmmimmmmmmmmmmv                                       \n");
    print(WRITETERMINAL, "                                       vmmmmmmmmmmmmmmmmmmmv                                        \n");
    print(WRITETERMINAL, "                                      rmmmmmmmmmmmmmmmmmmm                                          \n");
    print(WRITETERMINAL, "                                     mmmmmmmmmmmmmmmmmmmmv                                          \n");
    print(WRITETERMINAL, "                                 vrimmmmmmmmmmmmmmmmmmmv                                            \n");
    print(WRITETERMINAL, "                               rmmmmmmmmmmmmmmmmmmmmr                                               \n");
    print(WRITETERMINAL, "                            vrmmmmmmmmmmmmmmmmmmmmmm                                                \n");
    print(WRITETERMINAL, "                          vrmmmmmmmmmmmmmmmmmmmmmmmmr vv                                            \n");
    print(WRITETERMINAL, "                       vmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmv                                            \n");
    print(WRITETERMINAL, "                    vrmmmmmmmmmmmmmmmmmmmmmmmmmrv    vv                                             \n");
    print(WRITETERMINAL, "                rimmmmmmmmmmmmmmmmmmmmmmmmmmv v                                                     \n");
    print(WRITETERMINAL, "              vmmmmimmmmmmmmmmmmmmmmmmmmmvv                                                         \n");
    print(WRITETERMINAL, "             rmmmmmmmmmmmmmmmmmmmmmmmmmmv                                                           \n");
    print(WRITETERMINAL, "          vvmmmmmmmmmmmmmmmmmmmmmmmmmmmr                                                            \n");
    print(WRITETERMINAL, "         vrmmmmmmmmmmmmmmmmmmmmmmmmmmimr                                                            \n");
    print(WRITETERMINAL, "       rmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmivv                                                           \n");
    print(WRITETERMINAL, "     vmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm                                                              \n");
    print(WRITETERMINAL, "    vrrmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm v v                                                           \n");
    print(WRITETERMINAL, "   vmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm                                                               \n");
    print(WRITETERMINAL, "  rrrmmmmmmmmmmmmmmmmmmmmmmmmmmmmimv                                                                \n");
    print(WRITETERMINAL, " rrmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmrv                                                               \n");
    print(WRITETERMINAL, "vvrrrmmmmmmmmmmmmmmmmmmmmmmmmmmmr                                                                   \n");
    print(WRITETERMINAL, "  vrmmmmmrrmmmmmmmmmmmmmmmmmmmmr                                                                    \n");
    print(WRITETERMINAL, "      v    vmmmmmmmmmmmmmmmmmv                                                                      \n");
    print(WRITETERMINAL, "                vmmimmmmmmmmv                                                                       \n");
    print(WRITETERMINAL, "            v       vrrrv                                                                           \n");
    print(WRITETERMINAL, "            mmr                                                                                     \n");
    print(WRITETERMINAL, "           rmmmr                                                                                    \n");
    print(WRITETERMINAL, "        v vmrrv                                                                                     \n");
}

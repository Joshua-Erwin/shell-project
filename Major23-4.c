#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>


#define READ 0
#define WRITE 1

char* myHistory(char**, char*, char*, int);
char* myHistoryStore(char*, char*);
void alias(char**, char*, int);
char* isAlias(char**, char*, char*);
void fileSetup();
void changedir(char** arr);

int main(int argc, char* argv[])
{
    pid_t pid;
    pid_t batchpid;

    char prompt[100] = "Enter line->";
    char path[256];

    fileSetup();
    strcpy(path, getcwd(path, 256));
    //printf("%s opened successfully\n", path);

    

    int childStatus;
    //Batch Mode (maybe fork and set file to stdin)
    if (argc > 1) {
        pid = fork();
        if (pid != 0) {
            //printf("parent is %d\n", getpid());
            //printf("waiting for process %d\n", pid);
            waitpid(pid, &childStatus, 0);
            int isQuit = WEXITSTATUS(childStatus);
            if (isQuit == 1) {
                exit(0);
            }
            //printf("parent wake up\n");
        }
        else {
            batchpid = getpid();
            int batchfile;
            batchfile = open(argv[1], O_RDONLY, 0777);
            if (batchfile == -1) {
                printf("unable to open file");
                return -1;
            }
            else {
                printf("%s opened successfully\n", argv[1]);
                dup2(batchfile, STDIN_FILENO);
            }
        }
    }
    int numSemicolons = 0;
    int loops = 0;
    char* pastCommand = NULL;
    while (1)
    {
        char* userinput = malloc(512 * sizeof(char));
        char redirectInName[100];
        char redirectOutName[100];
        char* stringFormater;
        char* current = userinput;

        //Interactive Mode
        if (numSemicolons <= 0) {
            if (getpid() != batchpid && pastCommand == NULL) {
                printf("Enter line>");
            }
            if (pastCommand == NULL) {
                if (fgets(userinput, 512, stdin) == NULL) {
                    return 0;
                }
                userinput = myHistoryStore(userinput, path);
            }
            else {
                strcpy(userinput, pastCommand);
            }

            if (getpid() == batchpid) {
                printf("\nuserinput: %s\n", userinput);
            }


            stringFormater = userinput;

            //checks for empty line
            while (*stringFormater == ' ') {
                stringFormater++;
            }
            if (*stringFormater == '\n') {
                printf("no command entered\n");
                continue;
            }
            //checks for empty line end
        }
        else {
            stringFormater++;
            while (*stringFormater == ' ' && *stringFormater != '\0') {
                stringFormater++;
            }
            numSemicolons = 0;
        }
        current = stringFormater;
        int numArgs = 1;
        int numPipes = 0;
        int numRedirectIns = 0;
        int numRedirectOuts = 0;
        char* temp;

        //format input
        while (*stringFormater != '\0') {
            //printf("testing char: %c\n", *current);
            switch (*stringFormater) {
            case ' ':
                while (*stringFormater == ' ' && *stringFormater != '\0') {
                    stringFormater++;
                }
                numArgs++;
                break;
            case '|':
                numPipes++;
                stringFormater++;
                break;
            case '<':

                temp = stringFormater;
                *stringFormater = ' ';
                while (*stringFormater == ' ') {
                    stringFormater--;
                }
                numArgs--;
                while (*temp == ' ') {
                    temp++;
                }
                if (*temp != '\n') {
                    numRedirectIns++;
                    int i = 0;
                    while (*temp != ' ' && *temp != '\n' && *temp != ';') {
                        redirectInName[i++] = *temp;
                        *temp++ = ' ';
                    }
                    redirectInName[i] = '\0';
                }
                else {
                    printf("No inputfile given\n");
                }


                break;
            case '>':

                temp = stringFormater;
                *stringFormater = ' ';
                while (*stringFormater == ' ') {
                    stringFormater--;
                }
                numArgs--;
                while (*temp == ' ') {
                    temp++;
                }
                if (*temp != '\n') {
                    numRedirectOuts++;
                    int i = 0;
                    while (*temp != ' ' && *temp != '\n' && *temp != ';') {
                        redirectOutName[i++] = *temp;
                        *temp++ = ' ';
                    }
                    redirectOutName[i] = '\0';
                }
                else {
                    printf("No output file given\n");
                }

                break;
            case ';':
                temp = stringFormater;
                if (*(stringFormater - 1) != ' ') {
                    *stringFormater = '\0';
                }
                else {
                    *stringFormater = ' ';
                    numArgs--;
                }
                if (*(temp + 1) == '\n') {
                    *temp = '\0';
                }
                while (*(++temp) != '\n' && *(temp) != '\0') {
                    if (*temp == ';') {
                        *temp = ' ';
                    }
                    else if (*temp != ' ') {
                        numSemicolons++;
                        break;
                    }
                }
                while (*(stringFormater) == ' ') {
                    stringFormater--;
                }
                *(++stringFormater) = '\0';
                break;
            case '\n':
                *stringFormater = '\0';
                if (*(stringFormater - 1) == ' ' || *(stringFormater - 1) == '\0') {
                    numArgs--;
                }
                break;
            default:
                stringFormater++;
            }
        }

        //format array
        char** array = malloc(numArgs * sizeof(char*));
        int* pipeIndexes = malloc(numPipes * sizeof(int));
        array[0] = current;
        int i = 1;
        int pipeIndex = 0;
        int redirectIn = -1;
        int redirectOut = -1;
        int semiColon = -1;

        while (*current != '\0') {
            switch (*current) {
            case ' ':
                *current = '\0';
                while (*(current + 1) == ' ') {
                    current++;
                }
                if (*(current + 1) != '\0') {
                    array[i++] = ++current;
                }
                break;
            case '|':
                pipeIndexes[pipeIndex++] = i - 1;
                current++;
                break;
            case '<':
                redirectIn = i - 1;
                current++;
                break;
            case '>':
                redirectOut = i - 1;
                current++;
                break;
            case ';':
                semiColon = i - 1;
                current++;
                break;
            default:
                current++;
            }
        }

        if (strcmp(array[0], "myHistory") == 0 || strcmp(array[0], "myhistory") == 0) {
            pastCommand = myHistory(array, pastCommand, path, numArgs);
            if (pastCommand != NULL) {
                //printf("pastcommand: %s\n", pastCommand);
            }
            continue;
        }

        if (strcmp(array[0], "alias") == 0 || strcmp(array[0], "Alias") == 0) {
            alias(array, path, numArgs);
            continue;
        }

        if (strcmp(array[0], "cd") == 0)// cd function
        {
            changedir(array);
            continue;
        }

        pastCommand = isAlias(array, pastCommand, path);
        if (pastCommand != NULL) {
            //printf("pastcommand: %s\n", pastCommand);
            continue;
        }

        //check

        /*
        for (int i = 0; i < numArgs; i++) {
            printf("arg %d: %s\n", i, array[i]);
        }
        if (pipeIndex > 0) {
            printf("%s is a pipe at index %d\n", array[pipeIndex], pipeIndex);
        }
        if (numRedirectOuts != 0) {
            printf("redirectOutFileName: %s\n", redirectOutName);
        }
        if (redirectIn != -1) {
            printf("%s is a redirectIn at index %d\n", array[redirectIn], redirectIn);
        }
        if (semiColon != -1) {
            printf("%s is a semiColon at index %d\n", array[semiColon], semiColon);
        }


        if (numRedirectIns != 0) {
            printf("redirectInFileName: %s\n", redirectInName);
        }
        */

        int** pipesArray = malloc(numPipes * sizeof(int*));
        for (int i = 0; i < numPipes; i++) {
            pipesArray[i] = malloc(2 * sizeof(int));
            pipe(pipesArray[i]);
        }
        //execute piped commands
        if (numPipes > 0) {
            int startOfCommand;
            int endOfCommand;
            for (int i = 0; i <= numPipes; i++) {
                //build subCommand
                if (i == 0) { //first child
                    startOfCommand = 0;
                    endOfCommand = pipeIndexes[i] - 1;
                }
                else if (i < numPipes) { //middle children
                    startOfCommand = pipeIndexes[i - 1] + 1;
                    endOfCommand = pipeIndexes[i] - 1;
                }
                else { //last child
                    startOfCommand = pipeIndexes[i - 1] + 1;
                    endOfCommand = numArgs - 1;
                }

                //Build subcommand
                int size = (endOfCommand - startOfCommand) + 1;
                char** subCommand = malloc(size * sizeof(char*));
                for (int i = 0; i < size; i++) {
                    subCommand[i] = array[startOfCommand + i];
                }

                //fork and execute subcommand
                pid = fork();
                if (pid == 0) {
                    if (i == 0) { //First Child

                        if (numRedirectIns > 0) {
                            int inputFile = open(redirectInName, O_RDONLY, 0777);
                            if (inputFile == -1) {
                                printf("Failed to open file\n");
                                return -1;
                            }
                            else {
                                dup2(inputFile, STDIN_FILENO);
                                close(inputFile);
                            }
                        }

                        dup2(pipesArray[i][WRITE], 1);
                        for (int g = 0; g < numPipes; g++) {
                            close(pipesArray[g][READ]);
                            close(pipesArray[g][WRITE]);
                        }
                    }
                    else if (i < numPipes) { //Middle Children
                        dup2(pipesArray[i - 1][READ], 0);
                        dup2(pipesArray[i][WRITE], 1);

                        for (int g = 0; g < numPipes; g++) {
                            close(pipesArray[g][READ]);
                            close(pipesArray[g][WRITE]);
                        }
                    }
                    else { //Final Child

                        if (numRedirectOuts > 0) {
                            int inputFile = open(redirectOutName, O_WRONLY | O_CREAT, 0777);
                            if (inputFile == -1) {
                                printf("Failed to open output file\n");
                                return -1;
                            }
                            else {
                                dup2(inputFile, STDOUT_FILENO);
                                close(inputFile);
                            }
                        }

                        dup2(pipesArray[i - 1][READ], 0);
                        for (int g = 0; g < numPipes; g++) {
                            close(pipesArray[g][READ]);
                            close(pipesArray[g][WRITE]);
                        }

                    }
                    //execute command
                    if (execvp(subCommand[0], subCommand) == -1) {
                        printf("invalid command\n");
                        return -1;
                    }
                }
            }
            for (int g = 0; g < numPipes; g++) { //Parent Process Close and Wait
                close(pipesArray[g][READ]);
                close(pipesArray[g][WRITE]);
            }
            for (int i = 0; i <= numPipes; i++) {
                wait(NULL);
            }
        }
        else { //standard execution of single command

            //built in quit function
            if (strcmp(array[0], "exit") == 0) {
                exit(1);
            }

            pid = fork();
            if (pid == 0 && getpid() != batchpid) {

                //printf("standard execution by %d of userinput:%s\n", getpid(), userinput);
                char* emptyCheck = array[0];
                while (*emptyCheck == ' ') {
                    emptyCheck++;
                }
                if (*emptyCheck == '\n' || *emptyCheck == '\0') {
                    return 0;
                }

                if (numRedirectIns > 0) {
                    int inputFile = open(redirectInName, O_RDONLY, 0777);
                    if (inputFile == -1) {
                        printf("Failed to open input file\n");
                        return -1;
                    }
                    else {
                        dup2(inputFile, STDIN_FILENO);
                        close(inputFile);
                    }
                }

                if (numRedirectOuts > 0) {
                    int inputFile = open(redirectOutName, O_WRONLY | O_CREAT, 0777);
                    if (inputFile == -1) {
                        printf("Failed to open output file\n");
                        return -1;
                    }
                    else {
                        dup2(inputFile, STDOUT_FILENO);
                        close(inputFile);
                    }
                }

                if (execvp(array[0], array) == -1) {
                    printf("invalid command\n");
                    return -1;
                }
            }
            wait(NULL);
        }
        if (numSemicolons == 0) {
            free(userinput);
        }
        free(array);
        free(pipeIndexes);
        free(pipesArray);
        pastCommand = NULL;
    }

    return 0;
}

void changedir(char** arr)
{
    char cwd[256];
    if (arr[1] == NULL)
    {
        chdir(getenv("HOME"));
        printf("%s\n", getcwd(cwd, 256));
    }
    else
    {
        int num = chdir(arr[1]);
        if (num < 0)
        {
            printf("Not a directory\n");
        }
        printf("%s\n", getcwd(cwd, 256));
    }
}

char* myHistory(char** arr, char* returnStr, char* path, int args) {
    char filepath[512];
    strcpy(filepath, path);
    strcat(filepath, "/history.txt");

    char userstart[512];
    char userinputhistory[22][512];
    int historycount = 0;

    //checks for arguments
    if (args > 1) {
        //the clear command
        if (strcmp(arr[1], "-c") == 0) {
            int historyfile;
            historyfile = open(filepath, O_CREAT | O_TRUNC | O_WRONLY, 0777);
            if (historyfile == -1) {
                printf("unable to clear history file\n");
            }
            else {
                printf("history cleared successfully\n");
            }
            close(historyfile);
            return NULL;
        }
        //the execute command 
        else if (strcmp(arr[1], "-e") == 0 && args > 2 && args < 4 && atoi(arr[2]) > 0 && atoi(arr[2]) < 21) {

            FILE* history;
            history = fopen(filepath, "r");

            while (fgets(userstart, 512, history) != NULL) {
                strcpy(userinputhistory[historycount], userstart);
                historycount++;
            }

            fclose(history);

            int ofset;
            if (historycount == 21) {
                ofset = 1;
            }
            else
            {
                ofset = 0;
            }

            returnStr = malloc(strlen(userinputhistory[atoi(arr[2]) - ofset]) * sizeof(char));
            strcpy(returnStr, userinputhistory[atoi(arr[2]) - ofset]);
            //printf("what i need: %s",returnStr);
            return returnStr;

        }
        //if the command is wrong
        else {
            printf("not a valid command for myhistory.\n");
            printf("instead try:\n");
            printf("\tno arguments: show last 20 commands\n");
            printf("\t-e <history number>: execute one of the 20 commands in myhistory\n");
            printf("\t-c: clearing my history\n");
        }
        return NULL;
    }
    //no arguments
    else {

        FILE* history;
        history = fopen(filepath, "r");

        while (fgets(userstart, 512, history) != NULL) {
            strcpy(userinputhistory[historycount], userstart);
            historycount++;
        }

        fclose(history);

        for (int i = 1; i < historycount; i++) {
            printf("[%d]: %s", i, userinputhistory[i]);
        }
        return NULL;
    }
}


char* myHistoryStore(char* arr, char* path) {
    char filepath[512];
    strcpy(filepath, path);
    strcat(filepath, "/history.txt");

    char userstart[512];
    char userinputhistory[22][512];
    int historycount = 0;

    //open file to see if there is previous history
    FILE* history;
    history = fopen(filepath, "r");
    while (fgets(userstart, 512, history) != NULL) {
        strcpy(userinputhistory[historycount], userstart);
        historycount++;
    }
    strcpy(userinputhistory[historycount], arr);

    fclose(history);

    //open file to write down the new history as well as chop any extra
    history = fopen(filepath, "w");

    if (historycount > 20) {
        for (int i = 1; i < 22; i++) {
            fprintf(history, "%s", userinputhistory[i]);
        }
    }
    else {
        for (int i = 0; i < (historycount + 1); i++) {
            fprintf(history, "%s", userinputhistory[i]);
        }
    }
    fclose(history);
    return arr;
}


void alias(char** arr, char* path, int args) {
    char filepath[512];
    strcpy(filepath, path);
    strcat(filepath, "/alias.txt");

    char userstarts[512];
    char toBreakUp[512] = "";
    char nameofalias[20];
    char command[512];
    char* token;
    int numOfEqual = 0;
    int numOfQuotation = 0;
    
    FILE* alias;
    FILE* alias2;
    int exists = 0;
    int aliasfile;

    //printf("Filepath %s\n", filepath);
    
    //makes the sting to break up and counts = and ' 
    int c = 0;
    int j = 0; 
    for(int i = 0; i < args; i++)
    {   
        j = 0;
        while (arr[i][j] != '\0')
        {
            toBreakUp[c] = arr[i][j];
            if (arr[i][j] == '=')
            {
                numOfEqual++;
            }
            if (arr[i][j] == '\'')
            {
                numOfQuotation++;
            }
            //printf("inside %c\n", arr[i][j]);
            j++;
            c++;
        }
        toBreakUp[c] = ' ';
        c++;
    }

    //printf("Breakup %s\n", toBreakUp);

    //printf("[numOfEqual]:\t%d\n", numOfEqual);
    //printf("[numOfQuotation]:\t%d\n", numOfQuotation);

    //checks for arguments
    if (args > 1) {
        //the clear command
        if (strcmp(arr[1], "-c") == 0) {
            aliasfile = open(filepath, O_CREAT | O_TRUNC | O_WRONLY, 0777);
            if (aliasfile == -1) {
                printf("unable to clear alias file\n");
            }
            else {
                printf("alias cleared successfully\n");
            }
            close(aliasfile);
        }
        //the creating command 
        else if (numOfEqual == 1 && numOfQuotation == 2) {

            token = strtok(toBreakUp, "=");
            strcpy(nameofalias, token);
            token = strtok(NULL, "'");
            strcpy(command, token);
            token = strtok(nameofalias, " ");
            token = strtok(NULL, " ");
            strcpy(nameofalias, token);

            printf("The name of Alias: %s\n", nameofalias);
            printf("The command: %s\n", command);

            alias = fopen(filepath, "r");

            int linecount = 0;
            while (fgets(userstarts, 512, alias) != NULL) {
                token = strtok(userstarts, "=");
                if (strcmp(nameofalias, token) == 0) {
                    printf("The alias command %s already exists...\n", nameofalias);
                    exists = 1;
                }
                linecount++;
            }

            if (exists != 1) {
                alias = fopen(filepath, "a");
                if (linecount > 0) {
                    fprintf(alias, "\n");
                }
                fprintf(alias, "%s='%s'", nameofalias, command);
            }
            fclose(alias);

        }
        //the remove command 
        else if (strcmp(arr[1], "-r") == 0 && args > 2 && args < 4) {

            alias = fopen(filepath, "r");
            int linecount = 0;
            int linecount2 = 0;

            aliasfile = open("alias2.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
            close(aliasfile);

            while (fgets(userstarts, 512, alias) != NULL) {
                token = strtok(userstarts, "=");
                if (strcmp(arr[2], token) == 0) {
                    exists = 1;
                }
                if (exists != 1) {
                    linecount++;
                }
            }
            fclose(alias);

            if (exists != 1) {
                printf("The alias command %s does not exist...\n", arr[2]);
            }
            else {

                alias = fopen(filepath, "r");
                alias2 = fopen("alias2.txt", "w");
                while (fgets(userstarts, 512, alias) != NULL) {
                    if (linecount != linecount2) {
                        fprintf(alias2, "%s", userstarts);
                    }
                    linecount2++;
                }
                fclose(alias);
                fclose(alias2);

                alias = fopen(filepath, "w");
                alias2 = fopen("alias2.txt", "r");
                while (fgets(userstarts, 512, alias2) != NULL) {
                    fprintf(alias, "%s", userstarts);
                }
                fclose(alias);
                fclose(alias2);

                remove("alias2.txt");
                printf("The alias command %s has been removed...\n", arr[2]);
            }
        }
        //if the command is wrong
        else {

            printf("not a valid command for alias.\n");
            printf("instead try:\n");
            printf("\tno arguments: show all alias\n");
            printf("\talias_name='command': create a new alias\n");
            printf("\t-r alias_name: remove a single alias\n");
            printf("\t-c: clears all of alias\n");

        }
    }
    //no arguments
    else {

        alias = fopen(filepath, "r");

        while (fgets(userstarts, 512, alias) != NULL) {
            printf("%s", userstarts);
        }
        printf("\n");
        fclose(alias);
    }
}


char* isAlias(char** arr, char* returnStr, char* path) {
    char filepath[512];
    strcpy(filepath, path);
    strcat(filepath, "/alias.txt");

    FILE* alias;
    alias = fopen(filepath, "r");

    char userstart[512];
    char* token;

    while (fgets(userstart, 512, alias) != NULL) {
        token = strtok(userstart, "=");

        if (strcmp(arr[0], token) == 0) {
            token = strtok(NULL, "'");
            fclose(alias);

            // this is where the command would go to parse and run
            printf("execute %s\n", token);
            returnStr = malloc(strlen(token) * sizeof(char));
            strcpy(returnStr, token);
            return returnStr;
        }
    }
    fclose(alias);
    return NULL;
}

void fileSetup()
{   
    int bothfile;
    bothfile = open("history.txt", O_CREAT | O_WRONLY, 0777);
    close(bothfile);

    bothfile = open("alias.txt", O_CREAT | O_WRONLY, 0777);
    close(bothfile);

}
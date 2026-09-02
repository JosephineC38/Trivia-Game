/*******************************************************************************
 * Name        : server.c
 * Author      : Josephine Choong
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_CONN 3

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player {
    int fd;
    int score;
    char name[128];
};

int read_questions(struct Entry* arr, char* filename) {
    int count = 0;
    char line[256];

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while (count < 50) { 
        //prompt
        if (fgets(line, sizeof(line), fp) == NULL) {
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        strcpy(arr[count].prompt, line);

        //options
        if (fgets(line, sizeof(line), fp) == NULL) {
            break;
        }
        sscanf(line, "%s %s %s", arr[count].options[0], arr[count].options[1], arr[count].options[2]);
        
        //answer_idx
        if (fgets(line, sizeof(line), fp) == NULL) {
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, arr[count].options[0]) == 0) {
            arr[count].answer_idx = 0;
        } else if (strcmp(line, arr[count].options[1]) == 0) {
            arr[count].answer_idx = 1;
        } else if (strcmp(line, arr[count].options[2]) == 0) {
            arr[count].answer_idx = 2;
        }

        //white space
        fgets(line, sizeof(line), fp);

        count++;
    }

    fclose(fp);
    return count;
}


int main(int argc , char *argv[]){
    int  server_fd;
    int  client_fd = -1;
    char buffer[1025];
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);


    //default stuff 
    char* question_file = "qshort.txt";
    char* IP_address = "127.0.0.1";
    int port_number = 25555; 

     //parse args
    int opt;
    while((opt = getopt(argc, argv, ":f:i:p:h")) != -1) 
    { 
        switch(opt) 
        { 
            case 'f': 
                question_file = optarg;
                break;
            case 'i': 
                IP_address = optarg; 
                break;
            case 'p': 
                port_number = atoi(optarg);
                break; 
            case 'h': //change format
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
                printf("  -f question_file    Default to \"qshort.txt\";\n");
                printf("  -i IP_address       Default to \"127.0.0.1\";\n");
                printf("  -p port_number      Default to 25555;\n");
                printf("  -h                  Display this help info.\n"); 
                exit(EXIT_SUCCESS); 
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt); //check for addtional error
                exit(EXIT_FAILURE); 
        } 
    } 

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    int r = bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (r == -1) { perror("bind"); exit(-1); }

    //listen succeds
    if (listen(server_fd, 2) == 0) printf("Welcome to 392 Trivia!\n");
    else { printf("Error\n"); return 1; }

    //Task 3
    struct Player players[MAX_CONN]; 
    int ready = 0;
    bool gameStarted = false;

    //Task 4
    int currQuest = 0;
    bool firstAns = false;
       
    for (int i = 0; i < MAX_CONN; i++) {
        memset(&players[i], 0, sizeof(struct Player));
        players[i].fd = -1;
    }
    int curr_client = 0;
    char* msg = "Read\n";

    //Task 2
    struct Entry ques[50];
    int quesCount = read_questions(ques, question_file);
    

    while (1) {
        // Build the fd_set from scratch each iteration
        fd_set read_fds;
        FD_ZERO(&read_fds);

        // Always watch the server socket for new connections
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;

        // Watch each active client
        for (int i = 0; i < MAX_CONN; i++) {
            if (players[i].fd != -1) {
                FD_SET(players[i].fd, &read_fds);
                if (players[i].fd > max_fd)
                    max_fd = players[i].fd;
            }
        }

        // Block until at least one fd is ready
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) { perror("select"); break; }

        // New incoming connection?
        if (FD_ISSET(server_fd, &read_fds)) {
            addr_size = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &addr_size);
            if (client_fd < 0) { perror("accept"); continue; }

            if (curr_client < MAX_CONN) {
                for (int i = 0; i < MAX_CONN; i++) {
                    //Task 2
                    if (players[i].fd == -1) {
                        players[i].fd = client_fd;
                        curr_client++;
                        printf("New connection detected!\n");
                        char* name = "Please type your name: ";
                        write(players[i].fd, name, strlen(name));
                        break;
                    } 
                }
            } else {
                printf("Max connection reached!\n");
                close(client_fd);
            }
        }

        //Check each client for incoming data
        for (int i = 0; i < MAX_CONN; i++) {
            if (players[i].fd == -1) continue;
            if (!FD_ISSET(players[i].fd, &read_fds)) continue;

            int n = read(players[i].fd, buffer, sizeof(buffer) - 1);
            if (n == 0) {
                printf("Lost connection!\n");
                //close connections
                for (int j = 0; j < MAX_CONN; j++) {
                    close(players[j].fd);
                }
                close(server_fd);
                return 0;
            } else if (n < 0) {
                perror("read error");
                close(players[i].fd);
                players[i].fd = -1;
                curr_client--;
            } else {
                buffer[n] = 0;
                buffer[strcspn(buffer, "\n")] = '\0';

                if (strlen(players[i].name) == 0) {
                    if (strlen(buffer) == 0) {
                        continue;
                    }
                    strcpy(players[i].name, buffer);
                    printf("Hi %s!\n", players[i].name);
                    ready++;

                } else if (gameStarted && currQuest < quesCount) {    
                    if (firstAns == true) {
                        continue;
                    }
                    firstAns = true;

                    struct Entry entry = ques[currQuest];
                    int answer = atoi(buffer) - 1;
                    if ((answer) == entry.answer_idx) {
                        players[i].score++;
                    } else {
                        players[i].score--;
                    }

                    char answerStr[5000];
                    printf("The correct answer is %s\n\n", entry.options[entry.answer_idx]);
                    sprintf(answerStr, "The correct answer is %s\n\n", entry.options[entry.answer_idx]);
                    for (int i = 0; i < MAX_CONN; i++) {
                        write(players[i].fd, answerStr, strlen(answerStr));
                    }
                    currQuest++;
                    firstAns = false;

                    if (currQuest < quesCount) {
                        //Print questions to client
                        struct Entry entryNext = ques[currQuest];
                        char clientQuest[5000];
                        sprintf(clientQuest, "Question %i: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", currQuest + 1, entryNext.prompt, entryNext.options[0], entryNext.options[1], entryNext.options[2]);

                        //Print questions to server
                        printf("Question %i: %s\n", currQuest + 1, entryNext.prompt);
                        printf("1: %s\n", entryNext.options[0]);
                        printf("2: %s\n", entryNext.options[1]);
                        printf("3: %s\n", entryNext.options[2]);

                        for (int i = 0; i < MAX_CONN; i++) {
                            write(players[i].fd, clientQuest, strlen(clientQuest));
                        }

                    } else { //Game Completed

                        //Find Tie
                        int winner = -999;
                        for (int i = 0; i < MAX_CONN; i++) {
                            if (players[i].score > winner) {
                                winner = players[i].score;
                            }
                            //printf("%s Score: %i\n", players[i].name, players[i].score);
                        }

                        for (int i = 0; i < MAX_CONN; i++) {
                            if (players[i].score == winner) {
                                printf("Congrats, %s!\n", players[i].name);
                            }
                        }

                        //close connections
                        for (int i = 0; i < MAX_CONN; i++) {
                            close(players[i].fd);

                        }
                        close(server_fd);
                        return 0;

                    }                    
                } 
            }
        }

        //Everyone is connected
        if (ready == MAX_CONN) {
            printf("The game starts now!\n");

            //Print questions to client
            struct Entry entry = ques[0];
            char clientQuest[5000];
            sprintf(clientQuest, "Question 1: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", entry.prompt, entry.options[0], entry.options[1], entry.options[2]);

            //Print questions to server
            printf("Question 1: %s\n", entry.prompt);
            printf("1: %s\n", entry.options[0]);
            printf("2: %s\n", entry.options[1]);
            printf("3: %s\n", entry.options[2]);

            for (int i = 0; i < MAX_CONN; i++) {
                write(players[i].fd, clientQuest, strlen(clientQuest));
            }
            ready = 0;
            gameStarted = true;
        } 
        
    }
    
    close(server_fd);
    return 0;
}

  
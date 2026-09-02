/*******************************************************************************
 * Name        : client.c
 * Author      : Josephine Choong
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

 

void parse_connect(int argc, char** argv, int* server_fd) {
    //Default varibles
    char* IP_address = "127.0.0.1";
    int port_number = 25555;

    //parse args
    int opt;
    while((opt = getopt(argc, argv, ":i:p:h")) != -1) 
    { 
        switch(opt) 
        { 
            case 'i': 
                IP_address = optarg; 
                break;
            case 'p': 
                port_number = atoi(optarg); //check if valud?
                break; 
            case 'h': //change format
                printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n", argv[0]);
                printf("  -i IP_address       Default to \"127.0.0.1\";\n");
                printf("  -p port_number      Default to 25555;\n");
                printf("  -h                  Display this help info.\n"); 
                exit(EXIT_SUCCESS); 
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt); //check for addtional error
                exit(EXIT_FAILURE); 
        } 
    } 
    
    struct sockaddr_in tower_addr;
    socklen_t addr_size = sizeof(tower_addr);;
    /* STEP 1:
       Create a socket to talk to human beings on the earth;
       Set up socket for the human side;
    */
    *server_fd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&tower_addr, 0, sizeof(tower_addr));
    tower_addr.sin_family = AF_INET;
    tower_addr.sin_port = htons(port_number);
    tower_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 3:
       Try to connect to the tower on the earth.
    */
    connect(*server_fd, (struct sockaddr *) &tower_addr, addr_size);

}

int main(int argc , char *argv[]) {
    int alien_fd;
    char buffer[1024];

    parse_connect(argc, argv, &alien_fd);

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        // Always watch the server socket for new connections
        FD_SET(alien_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        int max_fd = alien_fd;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) { perror("select"); break; }

        //server
        if (FD_ISSET(alien_fd, &read_fds)) {
            int n = read(alien_fd, buffer, 1024); //read data from server
            if (n <= 0) {
                break;
            }
            buffer[n] = 0;

            printf("%s", buffer);
            fflush(stdout);
        }

        //stdin (user input)
        else if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int n = read(STDIN_FILENO, buffer, 1024);
            buffer[n] = 0;

            write(alien_fd, buffer, strlen(buffer));
        }  
    }

  close(alien_fd);
  return 0;
}

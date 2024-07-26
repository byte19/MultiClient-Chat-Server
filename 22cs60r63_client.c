#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>

#define SERVER_ADDRESS "127.0.0.4"
#define SERVER_PORT 8082
#define MAX_BUFFER_SIZE 1024

int main() {
    int sock_fd = 0;
    struct sockaddr_in server_address;
    char buffer[MAX_BUFFER_SIZE] = {0};
    char serv_msg[MAX_BUFFER_SIZE] = {0};
    char command[MAX_BUFFER_SIZE] = {0};
    int read_bytes = 0;
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    
    // create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // initialize server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    
    // convert IP address from text to binary format
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_address.sin_addr) <= 0) {
        perror("invalid server address");
        exit(EXIT_FAILURE);
    }
    
    // connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connection to server failed");
        exit(EXIT_FAILURE);
    }
    
    // read input from user and send to server
    int p=fork();
    if (p==0) {//write
            while (1) {
                bzero(command, MAX_BUFFER_SIZE);
                bzero(buffer, MAX_BUFFER_SIZE);
                fgets(command,1024, stdin);
                int num_bytes =0;
                // for(;command[num_bytes]!='\n';num_bytes++);
                // command[num_bytes] = '\0';
                send(sock_fd,command,strlen(command),0);
                //n = write(sock_fd,serv_msg,strlen(serv_msg));
                /*if (n < 0)
                    printf("ERROR writing to socket\n");
                else
                    printf("Write Request sent\n");*/
                if(!strcmp(serv_msg,"CLOSE\n")){
                    break;
                }
            }
    }
    else{//read
            while (1) {
                bzero(buffer, 1024);
                //printf("\xE2\x9C\x93: ");
                recv(sock_fd, buffer, sizeof(buffer), 0);
                if(strncmp(buffer,"CONNECTION TERMINATED",21)==0) break;
                //n = read(sock_fd,serv_response,256);
                printf("%s\n",buffer);
            }
    }
    close(sock_fd);
    printf("\nDisconnected from the server\n");
    return 0;
}

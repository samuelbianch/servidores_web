#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h> 

#define PORT 8080        
#define MAX_CLIENTS 30   
#define BUFFER_SIZE 1024 

int main() {
    int server_fd, new_socket, client_socket[MAX_CLIENTS], max_sd, sd, activity;
    int opt = 1;
    int addrlen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in address;

    fd_set readfds; 

    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Falha em setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Falha ao vincular");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    
    if (listen(server_fd, 3) < 0) {
        perror("Falha ao escutar");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor concorrente rodando na porta %d...\n", PORT);

    addrlen = sizeof(address);

    while (1) {
        
        FD_ZERO(&readfds);

        
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];

            
            if (sd > 0)
                FD_SET(sd, &readfds);

            
            if (sd > max_sd)
                max_sd = sd;
        }

        
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("Falha no select");
        }

        
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("Falha ao aceitar conexão");
                continue;
            }

            printf("Nova conexão aceita, socket %d\n", new_socket);

            
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adicionado ao índice %d\n", i);
                    break;
                }
            }
        }

        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                
                int bytes_read = read(sd, buffer, BUFFER_SIZE);
                if (bytes_read == 0) {
                    
                    printf("Cliente no socket %d desconectado\n", sd);
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    
                    buffer[bytes_read] = '\0';
                    printf("Mensagem do cliente no socket %d: %s\n", sd, buffer);

                    const char *response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 18\r\n\r\n"
                       "Mensagem recebida!";
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }

    return 0;
}

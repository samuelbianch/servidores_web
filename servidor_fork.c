#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT 8080        
#define MAX_CLIENTS 30   
#define BUFFER_SIZE 1024 

int main() {
    int server_fd, new_socket, client_socket[MAX_CLIENTS], addrlen;
    int opt = 1;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in address;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("falha em setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("falha ao vincular");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("fallha ao escutar");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("servidor com fork rodando na porta %d...\n", PORT);

    addrlen = sizeof(address);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("falha ao aceitar conexão");
            continue;
        }

        printf("nova conexão aceita, socket %d\n", new_socket);

        // criar um novo processo (fork) para lidar com a nova conexão
        pid_t pid = fork();

        if (pid == -1) {
            perror("falha ao criar processo filho");
            close(new_socket);
        } else if (pid == 0) {
            // processo filho
            close(server_fd);  // fechando socket no processo filho

            //dados do cliente
            int bytes_read;
            while ((bytes_read = read(new_socket, buffer, BUFFER_SIZE)) > 0) {
                buffer[bytes_read] = '\0';
                printf("mensagem do cliente no socket %d, do processo filho %d: %s\n", new_socket, getpid(), buffer);

                const char *response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 18\r\n\r\n"
                       "Mensagem recebida!";
                send(new_socket, response, strlen(response), 0);
            }

            if (bytes_read == 0) {
                printf("cliente no socket %d, processo filho %d, desconectado\n", new_socket, getpid());
            } else if (bytes_read == -1) {
                perror("erro ao ler dados");
            }

            //fechando processo filho
            close(new_socket);  
            exit(0);  
        } else {
            close(new_socket);  //fecheei o socket do cliente no processo pai
        }
    }

    return 0;
}

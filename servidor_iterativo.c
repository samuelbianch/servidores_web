#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080  
#define BUFFER_SIZE 1024  

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar socket");
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

    
    if (listen(server_fd, 1) < 0) {
        perror("Falha ao escutar");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor iterativo rodando na porta %d...\n", PORT);

    while (1) {
        printf("Aguardando conexão...\n");

        
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Falha ao aceitar conexão");
            continue;  
        }

        printf("Conexão aceita!\n");

        
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            printf("Mensagem recebida: %s\n", buffer);

            
            const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOlá, mundo!";
            write(client_fd, response, strlen(response));
        }

        
        close(client_fd);
        printf("Conexão encerrada.\n");
    }

    
    close(server_fd);
    return 0;
}

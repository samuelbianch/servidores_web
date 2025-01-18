#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080  // Porta para escutar
#define BUFFER_SIZE 1024  // Tamanho do buffer

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Criação do socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configuração do endereço
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincular o socket ao endereço e porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Falha ao vincular");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Colocar o socket em modo de escuta
    if (listen(server_fd, 3) < 0) {
        perror("Falha ao escutar");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor iterativo rodando na porta %d...\n", PORT);

    while (1) {
        printf("Aguardando conexão...\n");

        // Aceitar conexão do cliente
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Falha ao aceitar conexão");
            continue;  // Continua aguardando próximas conexões
        }

        printf("Conexão aceita!\n");

        // Receber dados do cliente
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            printf("Mensagem recebida: %s\n", buffer);

            // Enviar uma resposta
            const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOlá, mundo!";
            write(client_fd, response, strlen(response));
        }

        // Fechar o socket do cliente
        close(client_fd);
        printf("Conexão encerrada.\n");
    }

    // Fechar o socket do servidor (não será alcançado neste exemplo)
    close(server_fd);
    return 0;
}

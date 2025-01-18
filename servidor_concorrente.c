#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h> // Para lidar com errno e EINTR

#define PORT 8080        // Porta para escutar
#define MAX_CLIENTS 30   // Máximo de clientes suportados
#define BUFFER_SIZE 1024 // Tamanho do buffer

int main() {
    int server_fd, new_socket, client_socket[MAX_CLIENTS], max_sd, sd, activity;
    int opt = 1;
    int addrlen;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in address;

    fd_set readfds; // Conjunto de descritores monitorados

    // Inicializar os sockets dos clientes como 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Criar o socket principal
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o socket para reutilizar o endereço/porta
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Falha em setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincular o socket ao endereço
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

    printf("Servidor concorrente rodando na porta %d...\n", PORT);

    addrlen = sizeof(address);

    while (1) {
        // Limpar o conjunto de descritores
        FD_ZERO(&readfds);

        // Adicionar o socket principal ao conjunto
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Adicionar sockets de cliente ao conjunto
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];

            // Se o socket é válido, adiciona ao conjunto
            if (sd > 0)
                FD_SET(sd, &readfds);

            // Atualizar o maior descritor
            if (sd > max_sd)
                max_sd = sd;
        }

        // Esperar por atividade em algum dos sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("Falha no select");
        }

        // Verificar se há nova conexão no socket principal
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("Falha ao aceitar conexão");
                continue;
            }

            printf("Nova conexão aceita, socket %d\n", new_socket);

            // Adicionar o novo socket ao array de clientes
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adicionado ao índice %d\n", i);
                    break;
                }
            }
        }

        // Verificar atividade em sockets dos clientes
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                // Ler os dados do socket
                int bytes_read = read(sd, buffer, BUFFER_SIZE);
                if (bytes_read == 0) {
                    // Cliente desconectado
                    printf("Cliente no socket %d desconectado\n", sd);
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    // Responder ao cliente
                    buffer[bytes_read] = '\0';
                    printf("Mensagem do cliente no socket %d: %s\n", sd, buffer);

                    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nMensagem recebida!";
                    send(sd, response, strlen(response), 0);
                }
            }
        }
    }

    return 0;
}

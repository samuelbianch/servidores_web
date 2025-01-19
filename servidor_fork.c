#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h> // Para lidar com errno e EINTR

#define PORT 8080        // Porta para escutar
#define MAX_CLIENTS 30   // Máximo de clientes suportados
#define BUFFER_SIZE 1024 // Tamanho do buffer

int main() {
    int server_fd, new_socket, client_socket[MAX_CLIENTS], addrlen;
    int opt = 1;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in address;

    // Inicializar os sockets dos clientes como 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Criar o socket principal
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o socket para reutilizar o endereço/porta
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("falha em setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincular o socket ao endereço
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("falha ao vincular");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Colocar o socket em modo de escuta
    if (listen(server_fd, 3) < 0) {
        perror("falha ao escutar");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("servidor com fork rodando na porta %d...\n", PORT);

    addrlen = sizeof(address);

    while (1) {
        // Aceitar a nova conexão
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("falha ao aceitar conexão");
            continue;
        }

        printf("nova conexão aceita, socket %d\n", new_socket);

        // Criar um novo processo (fork) para lidar com a nova conexão
        pid_t pid = fork();

        if (pid == -1) {
            // Se houver erro ao criar o processo
            perror("calha ao criar processo filho");
            close(new_socket);
        } else if (pid == 0) {
            // Processo filho
            close(server_fd);  // Fechar o socket do servidor no processo filho

            // Ler dados do cliente
            int bytes_read;
            while ((bytes_read = read(new_socket, buffer, BUFFER_SIZE)) > 0) {
                buffer[bytes_read] = '\0';
                printf("mensagem do cliente no socket %d, do processo filho %d: %s\n", new_socket, getpid(), buffer);

                const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nMensagem recebida!\n";
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

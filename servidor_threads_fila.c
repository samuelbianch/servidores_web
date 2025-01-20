#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h> 
#include <sys/socket.h>
#include <errno.h>   /

#define PORT 8080        
#define MAX_CLIENTS 30  
#define MAX_THREADS 50    
#define BUFFER_SIZE 1024 

// Fila de tarefas
#define QUEUE_SIZE 10
int task_queue[QUEUE_SIZE];
int front = 0, rear = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Função para inserir socket na fila
void enqueue(int socket) {
    pthread_mutex_lock(&queue_mutex);
    if ((rear + 1) % QUEUE_SIZE == front) {
        printf("Fila cheia, aguardando...\n");
    } else {
        task_queue[rear] = socket;
        rear = (rear + 1) % QUEUE_SIZE;
        pthread_cond_signal(&queue_cond);  // Notificar uma thread para consumir
    }
    pthread_mutex_unlock(&queue_mutex);
}

// Função para remover socket da fila
int dequeue() {
    pthread_mutex_lock(&queue_mutex);
    while (front == rear) {
        pthread_cond_wait(&queue_cond, &queue_mutex);  // Espera por tarefa
    }
    int socket = task_queue[front];
    front = (front + 1) % QUEUE_SIZE;
    pthread_mutex_unlock(&queue_mutex);
    return socket;
}

void *handle_client(void *arg) {
    int new_socket;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while (1) {
        new_socket = dequeue();  // Obter um socket da fila

        // Processar a conexão
        while ((bytes_read = read(new_socket, buffer, BUFFER_SIZE)) > 0) {
            buffer[bytes_read] = '\0';
            printf("Mensagem do cliente no socket %d: %s\n", new_socket, buffer);
            const char *response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 18\r\n\r\n"
                       "Mensagem recebida!";
            send(new_socket, response, strlen(response), 0);
        }

        if (bytes_read == 0) {
            printf("Cliente no socket %d desconectado\n", new_socket);
        } else if (bytes_read == -1) {
            perror("Falha ao ler dados");
        }

        close(new_socket);  // Fechar o socket do cliente
    }

    return NULL;
}

int main() {
    int server_fd, new_socket, addrlen;
    int opt = 1;
    struct sockaddr_in address;
    pthread_t threads[MAX_THREADS]; 


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

    printf("Servidor com threads e fila de tarefas rodando na porta %d...\n", PORT);

    addrlen = sizeof(address);

    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, handle_client, NULL) != 0) {
            perror("Falha ao criar thread");
            exit(EXIT_FAILURE);
        }
    }

    while (1) {

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Falha ao aceitar conexão");
            continue;
        }

        printf("Nova conexão aceita, socket %d\n", new_socket);

        enqueue(new_socket);
    }

    return 0;
}

/*******************************************************************************
 * SERVIDOR no porto 9000, à escuta de novos clientes.  Quando surgem
 * novos clientes os dados por eles enviados são lidos e descarregados no ecran.
 * gcc -Wall -pthread ./tcp_server.c -o server
 *******************************************************************************/
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define BUF_SIZE 1024

typedef struct
{
    char usernames[2][30];
    int clients_fd[2];
    int mensagem_invalida;

    sem_t *mutex_user1;
    sem_t *mutex_user2;
    sem_t *mutex_login;

    char mensagem_user2[BUF_SIZE];
    char mensagem_user1[BUF_SIZE];

    pid_t childs_pid[2];

} SM;

int shm_id;
SM *shared_memory;

void process_client(int jogador);
void desiste(int jogador);
void erro(char *msg);
void terminar();

int main() {
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;
    int jogador = 0;
    int status;

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind");
    if (listen(fd, 2) < 0)
        erro("na funcao listen");

    client_addr_size = sizeof(client_addr);

    // Open shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(SM), IPC_CREAT | IPC_EXCL | 0700);

    // Attach
    shared_memory = shmat(shm_id, NULL, 0);

    sem_unlink("MUTEX_USER1");
    sem_unlink("MUTEX_USER2");
    sem_unlink("MUTEX_LOGIN");
    shared_memory->mutex_user1 = sem_open("MUTEX_USER1", O_CREAT | O_EXCL, 0700, 1);
    shared_memory->mutex_user2 = sem_open("MUTEX_USER2", O_CREAT | O_EXCL, 0700, 0);
    shared_memory->mutex_login = sem_open("MUTEX_LOGIN", O_CREAT | O_EXCL, 0700, 0);
    shared_memory->mensagem_invalida = 0;

    while (jogador < 2) {
        // clean finished child processes, avoiding zombies
        // must use WNOHANG or would block whenever a child process was working
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        // wait for new connection
        client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);

        // Store client socket
        shared_memory->clients_fd[jogador++] = client;

        printf("Client %d connected...\n", jogador);
        pid_t cpid = fork();
        if (cpid == 0) {
            process_client(jogador);
            close(fd);
            close(client);
            exit(0);
        }

        shared_memory->childs_pid[jogador - 1] = cpid;
    }

    wait(&status);
    if (WEXITSTATUS(status) == 1)
        kill(shared_memory->childs_pid[1], SIGKILL);
    else
        kill(shared_memory->childs_pid[0], SIGKILL);

    close(shared_memory->clients_fd[0]);
    close(shared_memory->clients_fd[1]);

    terminar();

    // close socket
    close(fd);

    return 0;
}

void process_client(int jogador) {
    char buffer[BUF_SIZE];

    int jogada = 0;

    snprintf(buffer, BUF_SIZE, "Indique o seu nome\nCliente%d: ", jogador);
    write(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);

    memset(buffer, 0, BUF_SIZE); // clean buffer
    read(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);

    strcpy(shared_memory->usernames[jogador - 1], buffer);
    memset(buffer, 0, BUF_SIZE);

    if (jogador == 1) {
        snprintf(buffer, BUF_SIZE, "Bem-vindo %s. À espera de outro jogador...\n", shared_memory->usernames[0]);
        write(shared_memory->clients_fd[0], buffer, BUF_SIZE);
    } else {
        snprintf(buffer, BUF_SIZE, "Bem-vindo %s. Vamos jogar!\n", shared_memory->usernames[1]);
        write(shared_memory->clients_fd[1], buffer, BUF_SIZE);
        sem_post(shared_memory->mutex_login);
    }

    while (1) {

        if (jogador == 1 && jogada == 0) { // User1 1 jogada

            sem_wait(shared_memory->mutex_login);

            snprintf(buffer, BUF_SIZE, "Insira uma nova palavra\nCliente1: ");
            write(shared_memory->clients_fd[0], buffer, BUF_SIZE);

            memset(buffer, 0, BUF_SIZE);
            read(shared_memory->clients_fd[0], buffer, BUF_SIZE);

            strcpy(shared_memory->mensagem_user1, buffer);
            sem_wait(shared_memory->mutex_user1);
            sem_post(shared_memory->mutex_user2);

            jogada++;
        } else {
            char palavra[BUF_SIZE - 500];    // para fazer o snprintf e ter espaco para alocar dentro do buffer
            char palavra_inserida[BUF_SIZE]; // palavra inserida pelo user
            int verificar_pal = 0;           // variavel pondo 1 se é valida se nao continua a 0

            memset(buffer, 0, BUF_SIZE); // limpar o buffer

            // semafros
            if (jogador == 1)
                sem_wait(shared_memory->mutex_user1);
            else
                sem_wait(shared_memory->mutex_user2);

            if (jogador == 1)
                strcpy(palavra, shared_memory->mensagem_user2);
            else
                strcpy(palavra, shared_memory->mensagem_user1);

            if (shared_memory->mensagem_invalida == 1) { // quando o outro user nao aceita a palavra
                shared_memory->mensagem_invalida = 0;

                if (jogador == 1) {

                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "%s considerou a sua palavra inválida. \n", shared_memory->usernames[1]);
                    write(shared_memory->clients_fd[0], buffer, BUF_SIZE);

                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "Insira uma nova palavra\nCliente1: ");
                    write(shared_memory->clients_fd[0], buffer, BUF_SIZE);

                } else {

                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "%s considerou a sua palavra inválida. \n", shared_memory->usernames[0]);
                    write(shared_memory->clients_fd[1], buffer, BUF_SIZE);

                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "Insira uma nova palavra\nCliente2: ");
                    write(shared_memory->clients_fd[1], buffer, BUF_SIZE);
                }

                memset(buffer, 0, BUF_SIZE);
                memset(palavra_inserida, 0, BUF_SIZE);
                read(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);
                strcpy(&palavra_inserida[0], buffer);

            } else {

                memset(buffer, 0, BUF_SIZE);

                if (jogador == 1) {
                    snprintf(buffer, BUF_SIZE, "%s enviou \"%s\"\n", shared_memory->usernames[1], palavra);
                } else {
                    snprintf(buffer, BUF_SIZE, "%s enviou \"%s\"\n", shared_memory->usernames[0], palavra);
                }

                write(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);

                memset(buffer, 0, BUF_SIZE);
                snprintf(buffer, BUF_SIZE, "Insira uma nova palavra\nCliente%d: ", jogador);
                write(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);

                memset(buffer, 0, BUF_SIZE);
                memset(palavra_inserida, 0, BUF_SIZE);
                read(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);
                strcpy(&palavra_inserida[0], buffer);
            }

            if (!strcmp(palavra_inserida, "DESISTO")) {
                desiste(jogador);
                exit(jogador);
            } else if (!strcmp(palavra_inserida, "INVALIDA")) {
                shared_memory->mensagem_invalida = 1;

            } else if ((palavra_inserida[0] == palavra[strlen(palavra) - 2]) && (palavra_inserida[1] == palavra[strlen(palavra) - 1])) {
                // aceita a palavra
            } else {

                while (verificar_pal == 0) {

                    if (!strcmp(palavra_inserida, "DESISTO")) {
                        desiste(jogador);
                        exit(jogador);
                    }

                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "Palavra errada\n");
                    write(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);

                    memset(buffer, 0, BUF_SIZE);
                    snprintf(buffer, BUF_SIZE, "Insira uma nova palavra\nCliente%d: ", jogador);
                    write(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);

                    memset(buffer, 0, BUF_SIZE);
                    memset(palavra_inserida, 0, BUF_SIZE);
                    read(shared_memory->clients_fd[jogador - 1], buffer, BUF_SIZE);
                    strcpy(&palavra_inserida[0], buffer);

                    if ((palavra_inserida[0] == palavra[strlen(palavra) - 2]) && (palavra_inserida[1] == palavra[strlen(palavra) - 1])) {
                        verificar_pal = 1;
                    }
                }
            }

            // semafros e atualiza palavras
            if (jogador == 1) {
                if (strcmp(palavra_inserida, "INVALIDA"))
                    strcpy(shared_memory->mensagem_user1, palavra_inserida);
                sem_post(shared_memory->mutex_user2);
            } else {
                if (strcmp(palavra_inserida, "INVALIDA"))
                    strcpy(shared_memory->mensagem_user2, palavra_inserida);
                sem_post(shared_memory->mutex_user1);
            }
        }
    }
}

void desiste(int jogador) {
    char buffer[BUF_SIZE];

    memset(buffer, 0, BUF_SIZE);
    if (jogador == 1) {
        snprintf(buffer, BUF_SIZE, "*\n*** %s ganhou ***\n", shared_memory->usernames[1]);
        write(shared_memory->clients_fd[0], buffer, BUF_SIZE);
        write(shared_memory->clients_fd[1], buffer, BUF_SIZE);
    }

    else {
        snprintf(buffer, BUF_SIZE, "*\n*** %s ganhou ***\n", shared_memory->usernames[0]);
        write(shared_memory->clients_fd[1], buffer, BUF_SIZE);
        write(shared_memory->clients_fd[0], buffer, BUF_SIZE);
    }
}

void erro(char *msg) {
    printf("Erro: %s\n", msg);
    exit(-1);
}

void terminar() {
    sem_close(shared_memory->mutex_user1);
    sem_close(shared_memory->mutex_user2);
    sem_close(shared_memory->mutex_login);
    sem_unlink("CHECK_PLAYERS");
    sem_unlink("MUTEX_USER1");
    sem_unlink("MUTEX_USER2");

    shmdt(shared_memory);
    shmctl(shm_id, IPC_RMID, NULL);
}
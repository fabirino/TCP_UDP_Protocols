/**********************************************************************
 * CLIENTE liga ao servidor (definido em argv[1]) no porto especificado
 * (em argv[2]), escrevendo a palavra predefinida (em argv[3]).
 * USO: >cliente <enderecoServidor>  <porto>  <Palavra>
 * gcc -Wall ./tcp_client.c -o cliente
 * ./cliente localhost 9000
 **********************************************************************/
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 1024
int fd;

void erro(char *mensagem);

void sigint(int num) {
    char buffer[BUF_SIZE];
    write(fd, "DESISTO", 20);
    recv(fd, buffer, BUF_SIZE, 0);

    printf("%s", &buffer[1]); // "\n**** user* ganhou****"
    close(fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    char endServer[100];
    struct sockaddr_in addr;
    struct hostent *hostPtr;

    if (argc != 3) {
        printf("cliente <host> <port>");
        exit(-1);
    }

    strcpy(endServer, argv[1]);
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Não consegui obter endereço");

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short)atoi(argv[2]));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("Connect");
    signal(SIGINT, SIG_IGN);

    int nread = 0;
    char buffer[BUF_SIZE];
    char client_no;
    int play_no = 0;

    nread = read(fd, buffer, BUF_SIZE);
    buffer[nread] = '\0';

    printf("Servidor: %s", buffer);
    client_no = buffer[26];

    memset(buffer, 0, BUF_SIZE);
    scanf("%s", &buffer[0]);
    write(fd, buffer, BUF_SIZE);

    memset(buffer, 0, BUF_SIZE);
    read(fd, buffer, BUF_SIZE);
    printf("Servidor: %s", buffer);

    // Game
    while (1) {

        if ((play_no == 0) && (client_no == '1')) { // primeira jogada do jogador 1

            memset(buffer, 0, BUF_SIZE); // limpar buffer
            read(fd, buffer, BUF_SIZE);
            printf("\nServidor: %s", buffer);

            memset(buffer, 0, BUF_SIZE);
            scanf("%s", &buffer[0]);
            write(fd, buffer, BUF_SIZE);

            play_no++;

        } else {

            memset(buffer, 0, BUF_SIZE);
            recv(fd, buffer, BUF_SIZE, 0);

            if ((buffer[0] == '*')) { // DESISTE
                printf("%s", &buffer[1]);
                close(fd);
                exit(0);
            }

            printf("\nServidor: %s", buffer);

            memset(buffer, 0, BUF_SIZE);
            recv(fd, buffer, BUF_SIZE, 0);
            printf("Servidor: %s", buffer);
            signal(SIGINT, sigint);
            memset(buffer, 0, BUF_SIZE);
            scanf("%s", &buffer[0]);
            write(fd, buffer, BUF_SIZE);
            signal(SIGINT, SIG_IGN);
        }
    }

    close(fd);
    exit(0);
}

void erro(char *mensagem) {
    printf("Erro: %s\n", mensagem);
    exit(-1);
}
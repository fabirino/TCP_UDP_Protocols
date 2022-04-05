#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512 // Tamanho do buffer

void erro(char *s) {
    perror(s);
    exit(1);
}

int main(int argc, char *argv[]) {
    char buf[BUFLEN];
    char palavra[20];
    struct sockaddr_in si_minha;// si_outra;

    int s, recv_len, send_len;
    socklen_t slen = sizeof(si_minha);

    if (argc != 4) {
        printf("cliente <endereço IP servidor> <porto do servidor> <palavra>\n");
        exit(1);
    }

    int PORT = atoi(argv[2]);
    strcpy(palavra, argv[3]);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((send_len = sendto(s, palavra, strlen(palavra), 0, (struct sockaddr *)&si_minha, slen)) == -1) {
        erro("Erro no sendto");
    }

    if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_minha, (socklen_t *)&slen)) == -1) {
        erro("Erro no recvfrom");
    }

    // Para ignorar o restante conteúdo (anterior do buffer)
    buf[recv_len] = '\0';
    printf("Recebi uma mensagem do servidor com o endereco %s e o porto %d\n", inet_ntoa(si_minha.sin_addr), ntohs(si_minha.sin_port));
    printf("%s\n", buf);

    close(s);
    exit(0);
}
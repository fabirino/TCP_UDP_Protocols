#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512 // Tamanho do buffer
#define PORT 9876  // Porto para recepção das mensagens

void erro(char *s) {
    perror(s);
    exit(1);
}

int verifica(char conjunto[][BUFLEN], char palavra[]) {
    for (int i = 0; i < 5; i++) {
        if (!strcmp(conjunto[i], palavra)) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
    char buf[BUFLEN];
    char conjunto[][BUFLEN] = {"ola", "fabio", "eduardo", "palavra", "saudade"};

    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    // Preenchimento da socket address structure
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1) {
        erro("Erro no bind");
    }

    while (strcmp(buf, "adeus")) {
        // Espera recepção de mensagem (a chamada é bloqueante)
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra, (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        // Para ignorar o restante conteúdo (anterior do buffer)
        buf[recv_len] = '\0';

        printf("Recebi uma mensagem do cliente com o endereço %s e o porto %d\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
        printf("Conteúdo da mensagem recebida: %s\n", buf);

        if(verifica(conjunto, buf)){
            char string[] = "A palavra recebida e reservada\n";
            printf("%s\n", string);
            if(sendto(s, string, strlen(string), 0, (struct sockaddr *)&si_outra, slen)==-1)
                erro("Erro no sendto");
        }else{
            char string[] = "A palavra recebida e nao reservada\n";
            printf("%s\n", string);
            if(sendto(s, string, strlen(string), 0, (struct sockaddr *)&si_outra, slen)==-1)
                erro("Erro no sendto");
        }

    }

    printf("O programa terminou!\n");
    // Fecha socket e termina programa
    close(s);
    return 0;
}
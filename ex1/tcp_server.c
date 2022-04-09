/*******************************************************************************
 * SERVIDOR no porto 9000, à escuta de novos clientes.  Quando surgem
 * novos clientes os dados por eles enviados são lidos e descarregados no ecran.
 *******************************************************************************/
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define BUF_SIZE 1024

void process_client(int fd,int count,struct sockaddr_in  client_addr);
void erro(char *msg);

int main() {
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind");
    if (listen(fd, 5) < 0)
        erro("na funcao listen");
    client_addr_size = sizeof(client_addr);
    int count = 0;

    while (1) {
        // clean finished child processes, avoiding zombies
        // must use WNOHANG or would block whenever a child process was working
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        // wait for new connection
        client = accept(fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_size);
        if (client > 0) {
            if (fork() == 0) {
                close(fd);
                count++;
                process_client(client,count,(struct sockaddr_in)client_addr);
                exit(0);
            }
            close(client);
        }
    }
    return 0;
}

void process_client(int client_fd,int count,struct sockaddr_in  client_addr) {
    int nread = 0;
    char buffer[BUF_SIZE];

    do {
        nread = read(client_fd, buffer, BUF_SIZE - 1);
        buffer[nread] = '\0';
        char  ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET,(struct sockaddr *)&client_addr.sin_addr,ip,INET_ADDRSTRLEN);
        printf("** New message received **\n");
        printf("Client %d connecting from (IP:port) %s:%d says %s\n",count,ip,client_addr.sin_port, buffer);
        char string[200];
        snprintf(string,200,"Server received connection from (IP:port) %s:%d; already received %d connections!\n",ip,client_addr.sin_port, count);
        write(client_fd,string ,200);
        fflush(stdout);
    } while (nread > 0);
    close(client_fd);
}

void erro(char *msg) {
    printf("Erro: %s\n", msg);
    exit(-1);
}

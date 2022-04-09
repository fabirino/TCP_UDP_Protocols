/**********************************************************************
 * CLIENTE liga ao servidor (definido em argv[1]) no porto especificado
 * (em argv[2]), escrevendo a palavra predefinida (em argv[3]).
 * USO: >cliente <enderecoServidor>  <porto>  <Palavra>
 **********************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

void erro(char *msg);

int main(int argc, char *argv[]) {
  char endServer[100];
  int fd;
  struct sockaddr_in addr;
  struct hostent *hostPtr;

  if (argc != 4) {
    printf("cliente <host> <port> <string>\n");
    exit(-1);
  }

  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Não consegui obter endereço");

  bzero((void *) &addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	  erro("socket");
  if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
	  erro("Connect");
  write(fd, argv[3], 1 + strlen(argv[3]));
  int nread = 0;
  char buffer[200];
  nread = read(fd, buffer, 199);
  buffer[nread] = '\0';
  printf("Received from server:\n");
  printf("%s",buffer);
  close(fd);
  exit(0);
}

void erro(char *msg) {
  printf("Erro: %s\n", msg);
	exit(-1);
}

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "basic.h"
#include "socket_helper.h"
#define EXIT_COMMAND "exit\n"

#define MAXLINE 4096
#define MAX(a,b) (((a)>(b))?(a):(b))


int vidas = 6;
int tamanho = 0;
char letra;

/* copiado do livro */
ssize_t Readline (int fd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n=1; n<maxlen; n++) {
        again:
        if ( (rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break; /* newline is stored, like fgets() */
        } else if (rc == 0) {
            if (n == 1)
                return (0); /* EOF, no data read */
            else
                break; /* EOF, some data was read */
        } else {
            if (errno == EINTR)
                goto again;
            return (-1); /* error, errno set by read() */
        }
    }

    *ptr = 0; /* null terminate like fgets() */
    return(n);
}

void doit(int sockfd);

int main(int argc, char **argv) {
  int    port, sockfd, opcao, retval;
  char * ip;
  char   error[MAXLINE + 1];
  struct sockaddr_in servaddr;
  fd_set rset;
  struct timeval tv;
  char recvline[MAXLINE];

  if (argc != 3) {
    strcpy(error,"uso: ");
    strcat(error,argv[0]);
    strcat(error," <IPaddress, Port>");
    perror(error);
    exit(1);
  }

  // setup
  ip = argv[1];
  port = atoi(argv[2]);
  sockfd = Socket(AF_INET, SOCK_STREAM, 0);
  servaddr = ClientSockaddrIn(AF_INET, ip, port);
  Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
  // recebe o tamanho da palavra do servidor
  Readline(sockfd, recvline, MAXLINE);
  sscanf(recvline, "%d", &tamanho);

  printf("\n\nWelcome to the Hangman game!\n \n \nPress 1 to start. \n");
  scanf("%d", &opcao);

  if (opcao == 1) {
    printf("\nType a letter and press Enter to see if the word contains this letter. To guess the word, type it and press Enter.\n");
    printf("\nThe game has begun!\n\n");
    doit(sockfd);
  }
  else {
    printf("Sorry, wrong option.\n");
    write(sockfd, EXIT_COMMAND, strlen(EXIT_COMMAND));
  }
   exit(0);
}

void doit(int sockfd) {
    fd_set fdset;
    int maxfds, i, j, l, letrasRestantes = tamanho, lines = 0, ch = 0, position = 0;
    char recvline[MAXLINE], rdline[MAXLINE], temp[MAXLINE], vetorPalavra[tamanho];
    bool esperandoResposta = false, letraUsada[26]; // 70 entrada no vetor letraUsada para incluir extended ascii

    FD_ZERO(&fdset);
    // seta as letras iniciais como vazias
    for (int i = 0; i < tamanho; i++)
      vetorPalavra[i] = '_';
    // setup do vetor de letras ja usadas
    for (int i = 0; i < 26; i++)
      letraUsada[i] = false;

    // loop principal
    while(vidas > 0) {
      printf("\n============================================\n\n");
      printf("The size of the word is: %d \n", tamanho);
      printf("You have %d lifes.\n", vidas);

      for (int i = 0; i < tamanho; i++) {
        printf("%c ", vetorPalavra[i]);
      }
      if (letrasRestantes == 0) {
        printf("\n\nCongratulations! You win!\n");
        write(sockfd, EXIT_COMMAND, strlen(EXIT_COMMAND));
        return;
      }
      printf("\n\nType a letter or try to guess the word:\n");

      // loop de leitura, envio e recebimento
      while(true) {
        FD_SET(STDIN_FILENO, &fdset);
        FD_SET(sockfd, &fdset);
        maxfds = MAX(STDIN_FILENO, sockfd) + 1;
        select(maxfds, &fdset, NULL, NULL, NULL);
        // atividade no STDIN, só lê se não estiver esperando resposta do servidor
        if (FD_ISSET(STDIN_FILENO, &fdset) && !esperandoResposta) {
          Readline(STDIN_FILENO, rdline, MAXLINE);
          // recebeu um caractere ascii e um \n
          if (strlen(rdline) == 2) {
              if (!isalpha(rdline[0])) { // não é uma letra
                  printf("\nType a letter or a word without spaces, please.\n\n");
                  continue; // volta a esperar input
              }
              rdline[0] = tolower(rdline[0]);
              letra = tolower(rdline[0]);
              l = letra-'a'; // converte pra posição na tabela ascii relativa ao caractere a
              if (letraUsada[l]) {
                printf("\nYou have already tried to use this letter! Please, try another one:\n");
                continue; // volta a esperar input
              }
              else letraUsada[l] = true; // marca como usada
          }
          // caractere não-ascii com \n
          if ( strlen(rdline) == 3 && !isalpha(rdline[0]) && !isalpha(rdline[1]) ) {
            printf("\nInvalid character! Type a letter or a word without spaces, please.\n");
            // tira vida e checa se acabou o jogo
            vidas--;
            if (vidas <= 0) {
              printf("You lose!\n");
              write(sockfd, EXIT_COMMAND, strlen(EXIT_COMMAND));
              return;
            }
            else
              printf("You have %d lifes.\n", vidas);
              printf("Type a letter or a word:\n");
            continue; // volta a esperar input
          }
          write(sockfd, rdline, strlen(rdline));
          if (strcmp(rdline, EXIT_COMMAND) == 0)
            return;
          esperandoResposta = true;
        }

        // atividade no socket
        if (FD_ISSET(sockfd, &fdset) && esperandoResposta) {
          Readline(sockfd, recvline, MAXLINE);
          i = 0;
          // itera em cima do que foi recebido para decodificar a string
          while (recvline[i] != '\n') {
            if (recvline[i] == '#') { // indicador de derrota
              vidas = -1;
              printf("This is not the correct word. You lose!\n");
              write(sockfd, EXIT_COMMAND, strlen(EXIT_COMMAND));
            }
            else if (recvline[i] == '!') { // indicador de vitoria
              vidas = -1;
              printf("Congratulations!! You win!\n");
              write(sockfd, EXIT_COMMAND, strlen(EXIT_COMMAND));
            }
            else if (recvline[i] == ';') { // separador na string de resposta
              i++;
              continue;
            }
            else if (recvline[i] == '0') { // letra chutada não existe na palavra
              vidas--;
            }
            else { // letra válida, recebeu uma lista de posições da letra enviada
              j = 0;
              memset(temp, 0, MAXLINE);
              temp[j] = recvline[i];
              while ((recvline[i+1] != ';') && (recvline[i+1] != '\n')) {  //lógica para posições com mais de um digito
                i++;
                j++;
                temp[j] = recvline[i];
              }
              sscanf(temp, "%d", &position);
              vetorPalavra[position-1] = letra;
              letrasRestantes--;
            }
            i++;
          }
          esperandoResposta = false;
          break;
        } // fim do socket
      } // fim do while true
    } // fim do while vidas > 0
    if (vidas != -1) {
      printf("You lose!\n");
      write(sockfd, EXIT_COMMAND, strlen(EXIT_COMMAND));
    }
}

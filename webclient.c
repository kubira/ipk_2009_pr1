#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <regex.h>
#define BUFFER_LEN 1000

int hlava;  //Jestli u¾ jsem pøijmul hlavièku

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {

  char *reg_vyraz = "^(http://)?([a-zA-Z0-9\\.-]+):?([0-9]{0,5})?/?([A-Za-z0-9\\/\\%\\=\\#\\@\\_\\.\\,\\+\\~\\?-]*)?$";  //RE
  char zprava[BUFFER_LEN];  //Øetìzec pro odesílanou zprávu serveru
  char casti[4][BUFFER_LEN] = {"","","",""};  //Øetìzce pro ulo¾ení èástí URL
  char buffer[BUFFER_LEN];  //Øetìzec pro chybu pøi rozdìlování URL adresy
  char soubor[BUFFER_LEN];  //Øetìzec pro název souboru
  char msg[BUFFER_LEN];  //Øetìzec pro pøijatou zprávu od serveru
  char head[BUFFER_LEN];  //Øetìzec pro hlavièku zprávy
  char stav[100];  //Øetìzec pro stavový kód stránky
  char *hIndex;  //Ukazatel na konec hlavièky
  struct sockaddr_in adresa;  //Struktura pro socket
  struct hostent *host;  //Struktura pro hosta
  regmatch_t re_struct[5];  //Struktura pro ulo¾ení øetìzcù z RE
  regex_t re_compiled;  //Zkompilovaný RE
  int status;  //Výsledek rozdìlení URL adresy
  int sock;  //Deskriptor socketu
  int index, x, t;  //Promìnné pro indexy, délku pøijatých dat
  int pozice = 0;  //Index pozice konce hlavièky
  FILE *p;  //Ukazatel na stahovaný soubor

  hlava = 0;  //Nastavím promìnnou hlava na 0, je¹tì jsem nepøijmul hlavièku

  if(argc < 2) {  //Pokud nebyl zadán druhý parametr (URL)
    fprintf(stderr, "Nebyla zadána URL adresa!!!\n");  //Vypí¹e se chyba
    return -1;  //A vrací se -1
  }

  if(regcomp(&re_compiled, reg_vyraz, REG_EXTENDED) != 0) {
  //Pokud se nepodaøila kompilace RE
    fprintf(stderr, "Chyba pøi kompilaci regulárního výrazu!!!\n");  //Chyba
    return -1;  //A vrací se -1
  }

  status = regexec(&re_compiled, argv[1], 5, re_struct, 0);  //Rozdìlení URL

  if(status == 0) {  //Pokud se rozdìlení URL podaøilo, ulo¾ím si výsledek
    strncpy(casti[0],argv[1]+re_struct[1].rm_so,re_struct[1].rm_eo-re_struct[1].rm_so);
    strncpy(casti[1],argv[1]+re_struct[2].rm_so,re_struct[2].rm_eo-re_struct[2].rm_so);
    strncpy(casti[2],argv[1]+re_struct[3].rm_so,re_struct[3].rm_eo-re_struct[3].rm_so);
    strncpy(casti[3],argv[1]+re_struct[4].rm_so,re_struct[4].rm_eo-re_struct[4].rm_so);
  } else {  //Kdy¾ se rozdìlení nezdaøilo
    regerror(status, &re_compiled, buffer, BUFFER_LEN);  //Zjistím proè
    printf("Chyba regulárního výrazu: %s\n", buffer);  //Vypí¹u chybu
    return -1;  //A vracím -1
  }

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
  //Pokud se nevytvoøil socket
    fprintf(stderr, "Nepodaøilo se vytvoøit socket!!!\n");  //Vypí¹i chybu
    return -1;  //A vracím -1
  }

  adresa.sin_family = PF_INET;  //Nastavím socket

  if(strlen(casti[2]) == 0) {  //Pokud nebyl zadán port
    adresa.sin_port = htons(80);  //Nastavím implicitnì 80
  } else {  //Pokud byl port zadán
    adresa.sin_port = htons(atoi(casti[2]));  //Nastavím ho
  }

  if((host = gethostbyname(casti[1])) == NULL) {
  //Pokud se nepodaøí zjistit hosta
    fprintf(stderr, "Chyba u funkce gethostbyname: %s!!!\n", casti[1]);  //Chyba
    return -1;  //A vracím -1
  }

  memcpy(&adresa.sin_addr, host->h_addr, host->h_length);  //Zkopíruji hosta

  if(connect(sock, (struct sockaddr *)&adresa, sizeof(adresa)) < 0 ) {
  //Pokud se nepodaøilo navázat spojení
    fprintf(stderr, "Chyba pøipojení!!!\n");  //Chyba
    return -1;  //A vracím -1
  }

///////////////////////////////////////////////////////////////////////////////////////////////////
  sprintf(zprava, "HEAD /%s HTTP/1.0\r\n\r\n", casti[3]);  //Zpráva pro server

  if(send(sock, zprava, strlen(zprava), 0) < 0 ) {
  //Pokud se nepodaøí poslat zprávu serveru
    fprintf(stderr, "Chyba pøi odeslání po¾adavku serveru!!!\n");  //Chyba
    return -1;  //A vracím -1
  }

  strcpy(msg, "");  //Vyprázdním øetìzec msg

  if((t = recv(sock, msg, BUFFER_LEN, 0)) <= 0) {
  //Pokud nepøi¹la ¾ádná data
    fprintf(stderr, "Nepodaøilo se pøijmout hlavièku!!!\n");  //Chyba
    return -1;  //Vracím -1
  }

  index = 0;  //Nastavím index na 0 pro dal¹í pou¾ití

  for(x = 0; x < t; x++) {  //Cyklus pro extrakci stavového kódu
    if(msg[x] != '\n') {  //Pokud není konec øádku
      stav[x] = msg[x];  //Ulo¾ím znak
    }
    else {  //Jinak ulo¾ím nulový znak
      stav[x] = '\0';
      break;  //Ukonèím cyklus
    }
  }

  //Oznámení chybných stavových kódù
  if(strstr(stav, "400")) {
    fprintf(stderr, "400 Bad Request\n");
    return -1;
  }
  if(strstr(stav, "401")) {
    fprintf(stderr, "401 Unauthorized\n");
    return -1;
  }
  if(strstr(stav, "403")) {
    fprintf(stderr, "403 Forbidden\n");
    return -1;
  }
  if(strstr(stav, "404")) {
    fprintf(stderr, "404 Not Found\n");
    return -1;
  }
  if(strstr(stav, "500")) {
    fprintf(stderr, "500 Internal Server Error\n");
    return -1;
  }
  if(strstr(stav, "501")) {
    fprintf(stderr, "501 Not Implemented\n");
    return -1;
  }
  if(strstr(stav, "502")) {
    fprintf(stderr, "502 Bad Gateway\n");
    return -1;
  }
  if(strstr(stav, "503")) {
    fprintf(stderr, "503 Service Unavailable\n");
    return -1;
  }

  if(strstr(msg, " 200 OK") != NULL) {  //Pokud je stavový kód 200 OK
      sprintf(zprava, "GET /%s HTTP/1.0\r\n\r\n", casti[3]);  //Zpráva pro server
  } else {  //Pokud nenastala chyba ani stav 200 OK, budu pøesmìrovávat
    for(x = 0; x < t; x++) {  //Cyklem hledám øetìzec Location
      if(msg[x] == 'L') {  //Pokud najdu L
        x++;  //Inkrementuji x
        if(msg[x] == 'o') {  //Pokud je dal¹í písmeno o
          x += 16;  //pøièítám 16 (cation: http://)
          while(msg[x] != '\r' && msg[x] != '/') {  //Extrahuji hosta
            head[index] = msg[x];  //Ukládám ho do øetìzce head
            x++;  //Inkrementuji x
            index++;  //Inkrementuji index
          }
          sprintf(zprava, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", casti[3], head);  //Zpráva pro server
          break;  //Ukonèení cyklu
        }
      }
      sprintf(zprava, "GET /%s HTTP/1.0\r\n\r\n", casti[3]);  //Zpráva pro server
    }
  }

  //printf("%s\n", zprava);

  strcpy(msg, "");
///////////////////////////////////////////////////////////////////////////////////////////////////

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
  //Pokud se nevytvoøil socket
    fprintf(stderr, "Nepodaøilo se vytvoøit socket!!!\n");  //Vypí¹i chybu
    return -1;  //A vracím -1
  }

  adresa.sin_family = PF_INET;  //Nastavím socket

  if(strlen(casti[2]) == 0) {  //Pokud nebyl zadán port
    adresa.sin_port = htons(80);  //Nastavím implicitnì 80
  } else {  //Pokud byl port zadán
    adresa.sin_port = htons(atoi(casti[2]));  //Nastavím ho
  }
  
  if(strlen(head) == 0) {
    if((host = gethostbyname(casti[1])) == NULL) {
    //Pokud se nepodaøí zjistit hosta
      fprintf(stderr, "Chyba u funkce gethostbyname: %s!!!\n", casti[1]);  //Chyba
      return -1;  //A vracím -1
    }
  } else {
    if((host = gethostbyname(head)) == NULL) {
    //Pokud se nepodaøí zjistit hosta
      fprintf(stderr, "Chyba u funkce gethostbyname: %s!!!\n", head);  //Chyba
      return -1;  //A vracím -1
    }
  }

  memcpy(&adresa.sin_addr, host->h_addr, host->h_length);  //Zkopíruji hosta

  if(connect(sock, (struct sockaddr *)&adresa, sizeof(adresa)) < 0 ) {
  //Pokud se nepodaøilo navázat spojení
    fprintf(stderr, "Chyba pøipojení!!!\n");  //Chyba
    return -1;  //A vracím -1
  }

  if(send(sock, zprava, strlen(zprava), 0) < 0 ) {
  //Pokud se nepodaøí poslat zprávu serveru
    fprintf(stderr, "Chyba pøi odeslání po¾adavku serveru!!!\n");  //Chyba
    return -1;  //A vracím -1
  }

  if(strlen(casti[3]) > 0) {  //Pokud je v URL název souboru
    for(x = (strlen(casti[3])-1); x >= 0; x--) {
    //Cyklem získám název souboru z jeho plné cesty
      if(casti[3][x] == '/') {  //Jak narazím na lomítko od konce cesty
        index = x;  //Ulo¾ím si index
        break;  //A ukonèím cyklus
      }
    }

    for(x = 0; casti[3][x] != '\0'; x++) {
    //Ulo¾ím si název souboru od indexu do konce øetìzce
      soubor[x] = casti[3][index+x+1];  //Ukládám po znacích
    }

    soubor[index+x+1] = '\0';  //Ukonèím øetìzec
  } else {  //Pokud není v URL název souboru
    strcpy(soubor, "index.html");  //Nastavím jej na index.html
  }

  if((p = fopen(soubor, "w")) == NULL) {  //Pokud se nepodaøilo otevøít soubor
    fprintf(stderr, "Soubor se nepodaøilo otevøít!!\n");  //Nastala chyba
    return -1;  //A vracím -1
  }

  strcpy(msg,"");  //Nastavím zprávu od serveru na prázdný øetìzec

  while((t = recv(sock, msg, BUFFER_LEN, 0)) > 0) {  //Cyklem pøijímám data

    if(hlava == 0) {  //Pokud jsem je¹tì nezpracoval hlavièku
      strcpy(head, msg);  //Ulo¾ím si hlavièku na pozdìji
      for(index = 0; index < t; index++) {  //Hledám ve zprávì její konec
        if(msg[index] == '\r') {  //Hledám znak \r
          if(msg[index+1] == '\n') {  //Za ním znak \n
            if(msg[index+2] == '\r') {  //Pak zase \r
              if(msg[index+3] == '\n') {  //A nakonec \n
                pozice = index + 4;  //Ulo¾ím si poslední výskyt konce hlavièky
                hlava = 1;  //Poznaèím si, ¾e u¾ hlavièka pøi¹la
              }
            }
          }
        }
      }
      if(strstr(soubor, "index.html")) hIndex = strstr(head, "\r\n\r\n");
      //Pokud budu ukládat stránku, hledám konec hlavièky v øetìzci head
      else hIndex = strstr(msg, "\r\n\r\n");
      //Jinak hledám v øetìzci msg
      fwrite((hIndex+4), (t-pozice), 1, p);  //Zapí¹u data za koncem hlavièky
    } else {  //Pokud u¾ mám zpracovanou hlavièku
      fwrite(msg, t, 1, p);  //Ukládám zprávu celou
    }
    strcpy(msg,"");  //Nastavím zprávu od serveru na prázdný øetìzec
  }

  if (fclose(p) == EOF) {  //Pokud se nepodaøilo uzavøít soubor
    fprintf(stderr, "Chyba pri zavirani souboru.\n");  //Nastala chyba
    return -1;  //A vracím -1
  }

  if (close(sock) < 0) {  //Pokud se nepodaøilo uzavøít socket
    fprintf(stderr, "Chyba pøi uzavírání socketu!!!\n");  //Nastala chyba
    return -1;  //A vracím -1
  }

  regfree(&re_compiled);  //Uvolním pamì» po RE

  return 0;  //Program skonèil bez chyby
}

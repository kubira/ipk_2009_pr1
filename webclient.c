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

int hlava;  //Jestli u� jsem p�ijmul hlavi�ku

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {

  char *reg_vyraz = "^(http://)?([a-zA-Z0-9\\.-]+):?([0-9]{0,5})?/?([A-Za-z0-9\\/\\%\\=\\#\\@\\_\\.\\,\\+\\~\\?-]*)?$";  //RE
  char zprava[BUFFER_LEN];  //�et�zec pro odes�lanou zpr�vu serveru
  char casti[4][BUFFER_LEN] = {"","","",""};  //�et�zce pro ulo�en� ��st� URL
  char buffer[BUFFER_LEN];  //�et�zec pro chybu p�i rozd�lov�n� URL adresy
  char soubor[BUFFER_LEN];  //�et�zec pro n�zev souboru
  char msg[BUFFER_LEN];  //�et�zec pro p�ijatou zpr�vu od serveru
  char head[BUFFER_LEN];  //�et�zec pro hlavi�ku zpr�vy
  char stav[100];  //�et�zec pro stavov� k�d str�nky
  char *hIndex;  //Ukazatel na konec hlavi�ky
  struct sockaddr_in adresa;  //Struktura pro socket
  struct hostent *host;  //Struktura pro hosta
  regmatch_t re_struct[5];  //Struktura pro ulo�en� �et�zc� z RE
  regex_t re_compiled;  //Zkompilovan� RE
  int status;  //V�sledek rozd�len� URL adresy
  int sock;  //Deskriptor socketu
  int index, x, t;  //Prom�nn� pro indexy, d�lku p�ijat�ch dat
  int pozice = 0;  //Index pozice konce hlavi�ky
  FILE *p;  //Ukazatel na stahovan� soubor

  hlava = 0;  //Nastav�m prom�nnou hlava na 0, je�t� jsem nep�ijmul hlavi�ku

  if(argc < 2) {  //Pokud nebyl zad�n druh� parametr (URL)
    fprintf(stderr, "Nebyla zad�na URL adresa!!!\n");  //Vyp�e se chyba
    return -1;  //A vrac� se -1
  }

  if(regcomp(&re_compiled, reg_vyraz, REG_EXTENDED) != 0) {
  //Pokud se nepoda�ila kompilace RE
    fprintf(stderr, "Chyba p�i kompilaci regul�rn�ho v�razu!!!\n");  //Chyba
    return -1;  //A vrac� se -1
  }

  status = regexec(&re_compiled, argv[1], 5, re_struct, 0);  //Rozd�len� URL

  if(status == 0) {  //Pokud se rozd�len� URL poda�ilo, ulo��m si v�sledek
    strncpy(casti[0],argv[1]+re_struct[1].rm_so,re_struct[1].rm_eo-re_struct[1].rm_so);
    strncpy(casti[1],argv[1]+re_struct[2].rm_so,re_struct[2].rm_eo-re_struct[2].rm_so);
    strncpy(casti[2],argv[1]+re_struct[3].rm_so,re_struct[3].rm_eo-re_struct[3].rm_so);
    strncpy(casti[3],argv[1]+re_struct[4].rm_so,re_struct[4].rm_eo-re_struct[4].rm_so);
  } else {  //Kdy� se rozd�len� nezda�ilo
    regerror(status, &re_compiled, buffer, BUFFER_LEN);  //Zjist�m pro�
    printf("Chyba regul�rn�ho v�razu: %s\n", buffer);  //Vyp�u chybu
    return -1;  //A vrac�m -1
  }

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
  //Pokud se nevytvo�il socket
    fprintf(stderr, "Nepoda�ilo se vytvo�it socket!!!\n");  //Vyp�i chybu
    return -1;  //A vrac�m -1
  }

  adresa.sin_family = PF_INET;  //Nastav�m socket

  if(strlen(casti[2]) == 0) {  //Pokud nebyl zad�n port
    adresa.sin_port = htons(80);  //Nastav�m implicitn� 80
  } else {  //Pokud byl port zad�n
    adresa.sin_port = htons(atoi(casti[2]));  //Nastav�m ho
  }

  if((host = gethostbyname(casti[1])) == NULL) {
  //Pokud se nepoda�� zjistit hosta
    fprintf(stderr, "Chyba u funkce gethostbyname: %s!!!\n", casti[1]);  //Chyba
    return -1;  //A vrac�m -1
  }

  memcpy(&adresa.sin_addr, host->h_addr, host->h_length);  //Zkop�ruji hosta

  if(connect(sock, (struct sockaddr *)&adresa, sizeof(adresa)) < 0 ) {
  //Pokud se nepoda�ilo nav�zat spojen�
    fprintf(stderr, "Chyba p�ipojen�!!!\n");  //Chyba
    return -1;  //A vrac�m -1
  }

///////////////////////////////////////////////////////////////////////////////////////////////////
  sprintf(zprava, "HEAD /%s HTTP/1.0\r\n\r\n", casti[3]);  //Zpr�va pro server

  if(send(sock, zprava, strlen(zprava), 0) < 0 ) {
  //Pokud se nepoda�� poslat zpr�vu serveru
    fprintf(stderr, "Chyba p�i odesl�n� po�adavku serveru!!!\n");  //Chyba
    return -1;  //A vrac�m -1
  }

  strcpy(msg, "");  //Vypr�zdn�m �et�zec msg

  if((t = recv(sock, msg, BUFFER_LEN, 0)) <= 0) {
  //Pokud nep�i�la ��dn� data
    fprintf(stderr, "Nepoda�ilo se p�ijmout hlavi�ku!!!\n");  //Chyba
    return -1;  //Vrac�m -1
  }

  index = 0;  //Nastav�m index na 0 pro dal�� pou�it�

  for(x = 0; x < t; x++) {  //Cyklus pro extrakci stavov�ho k�du
    if(msg[x] != '\n') {  //Pokud nen� konec ��dku
      stav[x] = msg[x];  //Ulo��m znak
    }
    else {  //Jinak ulo��m nulov� znak
      stav[x] = '\0';
      break;  //Ukon��m cyklus
    }
  }

  //Ozn�men� chybn�ch stavov�ch k�d�
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

  if(strstr(msg, " 200 OK") != NULL) {  //Pokud je stavov� k�d 200 OK
      sprintf(zprava, "GET /%s HTTP/1.0\r\n\r\n", casti[3]);  //Zpr�va pro server
  } else {  //Pokud nenastala chyba ani stav 200 OK, budu p�esm�rov�vat
    for(x = 0; x < t; x++) {  //Cyklem hled�m �et�zec Location
      if(msg[x] == 'L') {  //Pokud najdu L
        x++;  //Inkrementuji x
        if(msg[x] == 'o') {  //Pokud je dal�� p�smeno o
          x += 16;  //p�i��t�m 16 (cation: http://)
          while(msg[x] != '\r' && msg[x] != '/') {  //Extrahuji hosta
            head[index] = msg[x];  //Ukl�d�m ho do �et�zce head
            x++;  //Inkrementuji x
            index++;  //Inkrementuji index
          }
          sprintf(zprava, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", casti[3], head);  //Zpr�va pro server
          break;  //Ukon�en� cyklu
        }
      }
      sprintf(zprava, "GET /%s HTTP/1.0\r\n\r\n", casti[3]);  //Zpr�va pro server
    }
  }

  //printf("%s\n", zprava);

  strcpy(msg, "");
///////////////////////////////////////////////////////////////////////////////////////////////////

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
  //Pokud se nevytvo�il socket
    fprintf(stderr, "Nepoda�ilo se vytvo�it socket!!!\n");  //Vyp�i chybu
    return -1;  //A vrac�m -1
  }

  adresa.sin_family = PF_INET;  //Nastav�m socket

  if(strlen(casti[2]) == 0) {  //Pokud nebyl zad�n port
    adresa.sin_port = htons(80);  //Nastav�m implicitn� 80
  } else {  //Pokud byl port zad�n
    adresa.sin_port = htons(atoi(casti[2]));  //Nastav�m ho
  }
  
  if(strlen(head) == 0) {
    if((host = gethostbyname(casti[1])) == NULL) {
    //Pokud se nepoda�� zjistit hosta
      fprintf(stderr, "Chyba u funkce gethostbyname: %s!!!\n", casti[1]);  //Chyba
      return -1;  //A vrac�m -1
    }
  } else {
    if((host = gethostbyname(head)) == NULL) {
    //Pokud se nepoda�� zjistit hosta
      fprintf(stderr, "Chyba u funkce gethostbyname: %s!!!\n", head);  //Chyba
      return -1;  //A vrac�m -1
    }
  }

  memcpy(&adresa.sin_addr, host->h_addr, host->h_length);  //Zkop�ruji hosta

  if(connect(sock, (struct sockaddr *)&adresa, sizeof(adresa)) < 0 ) {
  //Pokud se nepoda�ilo nav�zat spojen�
    fprintf(stderr, "Chyba p�ipojen�!!!\n");  //Chyba
    return -1;  //A vrac�m -1
  }

  if(send(sock, zprava, strlen(zprava), 0) < 0 ) {
  //Pokud se nepoda�� poslat zpr�vu serveru
    fprintf(stderr, "Chyba p�i odesl�n� po�adavku serveru!!!\n");  //Chyba
    return -1;  //A vrac�m -1
  }

  if(strlen(casti[3]) > 0) {  //Pokud je v URL n�zev souboru
    for(x = (strlen(casti[3])-1); x >= 0; x--) {
    //Cyklem z�sk�m n�zev souboru z jeho pln� cesty
      if(casti[3][x] == '/') {  //Jak naraz�m na lom�tko od konce cesty
        index = x;  //Ulo��m si index
        break;  //A ukon��m cyklus
      }
    }

    for(x = 0; casti[3][x] != '\0'; x++) {
    //Ulo��m si n�zev souboru od indexu do konce �et�zce
      soubor[x] = casti[3][index+x+1];  //Ukl�d�m po znac�ch
    }

    soubor[index+x+1] = '\0';  //Ukon��m �et�zec
  } else {  //Pokud nen� v URL n�zev souboru
    strcpy(soubor, "index.html");  //Nastav�m jej na index.html
  }

  if((p = fopen(soubor, "w")) == NULL) {  //Pokud se nepoda�ilo otev��t soubor
    fprintf(stderr, "Soubor se nepoda�ilo otev��t!!\n");  //Nastala chyba
    return -1;  //A vrac�m -1
  }

  strcpy(msg,"");  //Nastav�m zpr�vu od serveru na pr�zdn� �et�zec

  while((t = recv(sock, msg, BUFFER_LEN, 0)) > 0) {  //Cyklem p�ij�m�m data

    if(hlava == 0) {  //Pokud jsem je�t� nezpracoval hlavi�ku
      strcpy(head, msg);  //Ulo��m si hlavi�ku na pozd�ji
      for(index = 0; index < t; index++) {  //Hled�m ve zpr�v� jej� konec
        if(msg[index] == '\r') {  //Hled�m znak \r
          if(msg[index+1] == '\n') {  //Za n�m znak \n
            if(msg[index+2] == '\r') {  //Pak zase \r
              if(msg[index+3] == '\n') {  //A nakonec \n
                pozice = index + 4;  //Ulo��m si posledn� v�skyt konce hlavi�ky
                hlava = 1;  //Pozna��m si, �e u� hlavi�ka p�i�la
              }
            }
          }
        }
      }
      if(strstr(soubor, "index.html")) hIndex = strstr(head, "\r\n\r\n");
      //Pokud budu ukl�dat str�nku, hled�m konec hlavi�ky v �et�zci head
      else hIndex = strstr(msg, "\r\n\r\n");
      //Jinak hled�m v �et�zci msg
      fwrite((hIndex+4), (t-pozice), 1, p);  //Zap�u data za koncem hlavi�ky
    } else {  //Pokud u� m�m zpracovanou hlavi�ku
      fwrite(msg, t, 1, p);  //Ukl�d�m zpr�vu celou
    }
    strcpy(msg,"");  //Nastav�m zpr�vu od serveru na pr�zdn� �et�zec
  }

  if (fclose(p) == EOF) {  //Pokud se nepoda�ilo uzav��t soubor
    fprintf(stderr, "Chyba pri zavirani souboru.\n");  //Nastala chyba
    return -1;  //A vrac�m -1
  }

  if (close(sock) < 0) {  //Pokud se nepoda�ilo uzav��t socket
    fprintf(stderr, "Chyba p�i uzav�r�n� socketu!!!\n");  //Nastala chyba
    return -1;  //A vrac�m -1
  }

  regfree(&re_compiled);  //Uvoln�m pam� po RE

  return 0;  //Program skon�il bez chyby
}

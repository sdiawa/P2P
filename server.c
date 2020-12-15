//
// Created by dsk on 04/12/2020.
//

#include "crypt.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include "functions.h"



int main(int argc, char *argv[])
{
    int socfd,confd,sock;	//socket file descriptor
    int slen, clen,serLen;	//length of  object
    struct sockaddr_in ser, cli,serv;	//object declaration of socket family
    struct User user = {.name ="",.password=""};
    time_t t;	//to store time value
    slen = sizeof(ser);
    serLen = sizeof(serv);
    clen = sizeof(cli);
    int portPartage;
    socfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socfd < 0)
    {
        printf("Erreur: socket non disponible.\n");
        return 0;
    }
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        printf("Erreur: socket non disponible.\n");
        return 0;
    }
    //initailisation du server distant
    cli.sin_family = AF_INET;       //family type

    //IP et Port aléatoires
    srand((unsigned) time(&t));
    cli.sin_port = htons(8000+rand()%100);
    cli.sin_addr.s_addr = INADDR_ANY;

    if(bind(socfd, (struct sockaddr*)&cli, clen) < 0)
    {
        printf("Erreur:  sync des sockets.\n");
        return 0;
    }
    //Infos server centralisé
    ser.sin_addr.s_addr = inet_addr("0.0.0.0");	//inet_addr convers string into dotted decimal ip
    ser.sin_port = htons(2020);

    //to store the server details
    ser.sin_family = AF_INET;

    //connection au serveur centraliser.

    if(connect(socfd, (struct sockaddr*)&ser, slen) < 0)
    {
        printf("Erreur: Le serveur centralié ne repond pas.\n");
        return 0;
    }

    printf("connection au serveur centralisé...\n");
    if (serverJoinCentral(socfd,cli,ser,slen,user) != 0){//tentative de connection avec le login et mp
        printf("mot de passe incorrect\n");
        return -1;
    }
    // Crée son propre sockect pour partager directement ave le client.

    //initialise le server
    serv.sin_family = AF_INET;	//type
    serv.sin_port = htons(8000+rand()%100);;
    serv.sin_addr.s_addr = cli.sin_addr.s_addr;
    serLen = sizeof(serv);
    portPartage = ntohs(serv.sin_port);
    if(bind(sock, (struct sockaddr*)&serv, serLen) < 0)
    {
        printf("Error de Sync des Serveurs.\n");
        return 0;
    }


    struct sockaddr_in servCli;	//to store requesting client info
    listen(sock, 4);//ecoute du socket

    //envoie des infos du fichier au serveur centraliser
    signal(SIGCHLD, SIG_IGN);
    printf("Votre serveur %s %hu/%hu a bien démarrer \n",inet_ntoa(serv.sin_addr),ntohs(cli.sin_port), ntohs(serv.sin_port));
    int status = 1;
    FILE *d;
    int cmd = 0;
    char filename[50];
    char fileDesc[256];
    char buff[1024];
    int founded;
    struct stat file_stat;
    while (1){
        bzero(filename,50);
        bzero(fileDesc,256);
        recv(socfd,&status, sizeof(int),0);
        if (status == 0){
            printf("erreur interne\n");
            close(socfd);
            break;
        }
        printf("Evoyer un fichier ? (oui 1 / non 0)\n");
        scanf("%d",&cmd);

        if (cmd == 1){
            //partager un fichier
            while (strlen(filename) <=0){
                printf("Entrer le nom du fichier a partagé:\n");
                scanf("%s",filename);
            }

            d = fopen(filename, "a");
            if(!d){
                printf("Erreur:  aucun fichier detecté.\n");
                continue;
            }
            stat(filename,&file_stat);
            if (S_ISREG(file_stat.st_mode))
            {
                printf("Entrer la description de votre fichier:\n");
                getchar();
                fgets(fileDesc,256,stdin);
                send(socfd, fileDesc, 256, 0);
                send(socfd, filename, 50, 0);
                send(socfd,&portPartage,sizeof(int),0);
                status = 200;
                send(socfd,&status,sizeof(int),0);
                recv(socfd,&status,sizeof(int),0);
                if (status == 1)
                    printf("fichier envoyé avec success.\n");
                else{
                    printf("fichier non envoyé.\n");
                }
                fclose(d);
                while (1){
                        //attente du client pour envoie
                        printf("attente d'un client.\n");
                        confd = accept(sock, (struct sockaddr*)&servCli, &serLen);//respond to the request made by client
                        printf("Un client se connecte...\n");
                        if(confd < 0)
                        {
                            printf("La connexion a échoué.\n");
                            return -1;
                        }
                        //send the file
                        bzero(filename,50);
                        founded = -1;
                        recv(confd,filename,50,0);
                        printf("ouverture du fichier %s\n",filename);
                        FILE *fd = fopen(filename,"r");
                        if (!fd){
                            printf("Ce fichier est introuvable.\n");
                            send(confd,&founded, sizeof(int ),0);
                            continue;
                        }
                        founded = 1;
                        printf("Envoie du ficher %s vers %s %hu\n",filename,inet_ntoa(servCli.sin_addr),ntohs(servCli.sin_port));
                        while(fread(buff,1, sizeof(buff), fd) >0)
                        {
                            founded = 1;
                            //bzero(buff, 1024);
                            send(confd, &founded, sizeof(int), 0);
                            send(confd, buff, sizeof(buff), 0);
                        }
                        founded = 0;
                        send(confd, &founded, sizeof(int), 0);
                        fclose(fd);
                        close(confd);
                    }
                }
            else {
                printf("impossible d'envoyer ce type de fichier.\n");
            }
            fclose(d);
        } else{
            //recv(socfd,&status, sizeof(int),0);
            break;
        }

    }
    close(sock);
    close(socfd);
    return 1;
}
//login au server centralisé
int serverJoinCentral(int socfd, struct sockaddr_in cli, struct sockaddr_in ser, int slen, User user)
{
    printf("Connection au serveur\n");
    int mode = 0;
    int port = ntohs(cli.sin_port);
    char *ip = inet_ntoa(cli.sin_addr);
    int match;
    char toSend[1024];
    printf("Entrer votre pseudo\n");
    fgets(user.name,128,stdin);
    printf("Entrer votre mot de passe\n");
    fgets(user.password,128,stdin);
    strcpy(toSend,user.name);
    strcat(toSend,crypt(user.password,"Ex>Tn2O6oBJ=2pmSZeG9zS/59|[GE<*(_&t^VlY$S-W2>!/=2RJp* ?;cpoOr|uy"));
    send(socfd, &(mode), sizeof(int), 0);
    send(socfd,toSend, 1024,0);
    recv(socfd, &match, sizeof(int), 0);
    //printf("%d match\n",match);
    if (match == 403){
        printf("acces réfusé, :( mot de passe.\n");
        return -1;
    }
    if (match == -1){
        printf("Erreur: username ou mot de passe incorrect.\n");
        return -1;
    }
    if (match == 201){
        printf("Nouveau compte detecter pour %s",user.name);
        return 0;
    }
    return 0;
}


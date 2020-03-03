#include "server.h"

#include <string>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <iostream>

void Server::start()
{
    /* Handle connections */
    while(1) {
        struct sockaddr_in addr;
        uint len = sizeof(addr);
        SSL *ssl;

        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0)
        {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }

        pid=fork();
        if(pid==0)
        {
            ssl = SSL_new(ctx);
            SSL_set_fd(ssl, client);

            if (SSL_accept(ssl) <= 0)
            {
                ERR_print_errors_fp(stderr);
            } 
            else
            {
                handleClient(ssl);
                /*const char reply[] = "1";
                char readBuffer[10];
                SSL_read(ssl,readBuffer,10);
                SSL_write(ssl, reply, strlen(reply));*/
                signal(SIGCHLD,SIG_IGN);
                SSL_shutdown(ssl);
                SSL_free(ssl);
                exit(0);
            }
        }
        else
        {
            

            close(client);
        }
    }

    close(sock);
    SSL_CTX_free(ctx);
    cleanupSSL();
}

void Server::handleClient(SSL* ssl)
{
    //buffer size can be changed to whatever
    int bufferSize = 255;
    std::string reply = "";
    char readBuffer[bufferSize];
    SSL_read(ssl,readBuffer,bufferSize);
    std::vector<std::string> variables;
    variables = parseBuffer(readBuffer,bufferSize);
    Email email(variables);
    int emailSuccess = email.send();
    //emailSuccess = -1 means the system call failed
    if(emailSuccess >= 0)
    {
        reply = "1";
    }
    else
    {
        reply = "0";
    }
    SSL_write(ssl,&reply[0],1);
    /*const char asd[] = "1";
    int test = SSL_write(ssl, asd, strlen(asd));
    std::cout << test << "\n";*/
}

int Server::sendEmail(std::vector<std::string> variables)
{
    
    std::vector<std::string> stringVariables;
    for(size_t i=1;i<variables.size();i++)
    {
        stringVariables.push_back(variables.at(i));
    }
    
}

Server::Server(int port)
{
    initOpenSSL();
    ctx = createContext();
    configureContect(ctx);
    sock = createSocket(port);
}

std::vector<std::string> Server::parseBuffer(char* buffer,int length)
{
    std::vector<std::string> stringVector;
    std::string tempString = "";

    //0 is invite, 1 is forgotten password
    //int code = (int)buffer[0]-48;

    for(int i=0;i<length;i++)
    {
        char tempChar = buffer[i];
        if(tempChar=='\n')
        {
            stringVector.push_back(tempString);
            tempString = "";
        }
        else
        {
            tempString += tempChar;
        }

    }

    return stringVector;

}

int Server::createSocket(int port)
{
    int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(s, 1) < 0)
    {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }

    return s;
}

void Server::initOpenSSL()
{ 
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void Server::cleanupSSL()
{
    EVP_cleanup();
}

SSL_CTX* Server::createContext()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void Server::configureContect(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}
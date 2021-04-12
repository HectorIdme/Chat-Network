/* Client code in C++ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <thread>

using namespace std;
int SocketFD;

void Connection ( string IP, int port)
{
  struct sockaddr_in stSockAddr;
  SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  int Res;
  int n;

  if (-1 == SocketFD){
      perror("cannot create socket");
      exit(EXIT_FAILURE);
    }else{printf("socket sucessfully created\n");}

   memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
 
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);
    Res = inet_pton(AF_INET, IP.c_str() , &stSockAddr.sin_addr);

    if (0 > Res)
    {
      perror("error: first parameter is not a valid address family");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    else if (0 == Res)
    {
      perror("char string (second parameter does not contain valid ipaddress");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
 
    if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
    {
      perror("connect failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }else{printf("connected to the server\n");}
}

void Listen()
{ 
  while(1)
  {
    int n;
    char buffer[256];
    bzero(buffer,256);
    n = read(SocketFD, buffer,256);
    if (n < 0) perror("ERROR reading from socket");
    cout<<buffer<<endl;
  }
}


int main(void)
{
  Connection("100.107.106.165",50001);
  int n;
  thread(Listen).detach();
  while( 1 )
  { 
      string message;
      //cout<<"Type a message: ";
      message="";
      getline(cin,message);
      n = write(SocketFD,message.c_str(),message.length());      
  }

  shutdown(SocketFD, SHUT_RDWR);

  close(SocketFD);
  return 0;
}

/* Client code in C++ */
//g++ client.cpp -o client.exe -lpthread
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <thread>

using namespace std;
int SocketFD;
string comando;
bool salida = false;

////////////////////////////////////////////
////////////////////////////////////////////

int getSize(int s){
    string tam = "9";
    int res;
    for(int i=1;i<s;i++){tam += "9";}
    res = stoi(tam);
    return res;
}

int len(char* w){
    int count =0;
    while(*w != '\0'){
        count++;
        w++;
    }
    return count;
}

string formatNumbers(int range,int number){
    string numb = to_string(number);
    if(numb.size() < range){
        for(int i=1;i<range;i++){
            numb.insert(0,"0");
        }
    }
    return numb;
}


struct login{
  char accion;
  char tamanio_user[2];
  char tamanio_password[2];
  char* user;
  char* password;

  login(){
    size_t tam_user = sizeof(tamanio_user)/sizeof(tamanio_user[0]);
    size_t tam_pass = sizeof(tamanio_password)/sizeof(tamanio_password[0]);
    accion = 'l';
    user = new char[getSize(tam_user)];
    password = new char[getSize(tam_pass)];
  }

  void loginView(){
    cout<<"LOGIN"<<endl;
    cout<<"Username: "; cin>>user;
    cout<<"Password: "; cin>>password;
  }

  void print(){
      cout<<user<<endl;
      cout<<password<<endl;
  }

  string make_packet(){

    size_t tam_user = sizeof(tamanio_user)/sizeof(tamanio_user[0]);
    size_t tam_pass = sizeof(tamanio_password)/sizeof(tamanio_password[0]);
    int numberChar_user = len(user);
    int numberChar_pass = len(password);

    string user_size = formatNumbers(tam_user,numberChar_user);
    string pass_size = formatNumbers(tam_pass,numberChar_pass);

    string packet = "";

    packet += accion;
    packet += user_size;
    packet += pass_size;
    packet += user;
    packet += password;

    return packet;
  }

};

struct list{
  char accion;

  list(){
    accion = 'i';
  }

  char make_packet(){
    return accion;
  }
};


struct salir{
  char accion;

  salir(){
    accion = 'x';
  }

  char make_packet(){
    return accion;
  }
};


bool getDataFromPacket_Okey(string msg){
  if(msg[0] != 'L'){return false;}
  msg.erase(0,1);
  cout<<msg<<endl;
  return true;
}

bool getDataFromPacket_List(string msg){
  if(msg[0] != 'I'){return false;}

  vector<string> users;
  vector<int> tamanios;
  string u = "";
  string tamU,numCli;
  int tam_u,num_clients;

  msg.erase(0,1);

  numCli = msg.substr(0,2);
  num_clients = stoi(numCli);

  msg.erase(0,2);

  for(int i=0;i<num_clients;i++){
    tamU = msg.substr(0,2);
    tam_u = stoi(tamU);

    tamanios.push_back(tam_u);
    msg.erase(0,2);
  }

  for(int i=0;i<num_clients;i++){
    u = msg.substr(0,tamanios[i]);
    users.push_back(u);
    msg.erase(0,tamanios[i]);
  }
  
  for(int i=0;i<users.size();i++){
    cout<<"[-]"<<users[i]<<endl;
  }

  if(msg.size() == 0){return true;}
  else{return false;}
}



////////////////////////////////////////////
////////////////////////////////////////////

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
  char b[256];
  bzero(b,256);
  read(SocketFD, b,256);
  bool validate_login = getDataFromPacket_Okey(b);
  cout<<"login: "<<validate_login<<endl;

  if(validate_login){
    while(1)
    {
      int n;
      char buffer[256];
      bzero(buffer,256);
      n = read(SocketFD, buffer,256);
      if (n < 0){
        //perror("ERROR reading from socket");
        break;
      }
      else if(comando == "i"){
        getDataFromPacket_List(buffer);
      }
      else if(comando == "x"){
        if(strcmp(buffer,"X")==0){
          salida = true;
        }
      }
      else{
        cout<<buffer<<endl;
      }
    }
  }
}


int main(void)
{
  Connection("127.0.0.1",45000);
  int n;
  

  login u;
  u.loginView();
  string msg = u.make_packet();
  write(SocketFD,msg.c_str(),msg.size());

  thread(Listen).detach();

  while( 1 )
  { 
      if(salida){break;}
      string message;
      cout<<">>";
      message="";
      getline(cin,message);
      comando = message;
      
      if(comando == "i"){
        list l;
        message = l.make_packet();
      }
      else if(comando == "x"){
        salir e;
        message = e.make_packet();
      }
      n = write(SocketFD,message.c_str(),message.length());
      if (n < 0){
        //perror("ERROR reading from socket");
        break;
      }    
  }

  

  shutdown(SocketFD, SHUT_RDWR);

  close(SocketFD);
  return 0;
}

  /* Server code in C */
  //g++ server.cpp -o server.exe -lpthread
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <iostream>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
  #include <utility>
  #include <regex>
  #include <thread>
  #include <vector>

  using namespace std;
 

//clientes formato -> eg:  <3,[nom_usuario,password]>
vector<pair<int,vector<string>>> clientes;
string pass_general = "ucsp";
struct error;
////////////////////////////////////////////
////////////////////////////////////////////

/////////////
//STRUCTURE FOR ERRORS
/////////////

//error: detecta errores en el programa
struct error{
  char accion;
  char error_msg[20];

  error(){
    accion = 'E'; 
  }

  void set_msg(string msg){
    strcpy(error_msg,msg.c_str());
  }  

  string make_packet(){
    string packet = " ";
    
    packet += accion;
    packet += error_msg;

    return packet;
  }
};


////////////
//FUNCTIONS
////////////


//getUser: Obtener el nombre de usuario a partir del id-socket en vector clientes
string getUser(int id){
  string user;
  for(int i=0;i<clientes.size();i++){
    if(clientes[i].first == id){
        user = clientes[i].second[0];
        break;
    }
  }
  return user;
}

//getID: Obtener el id-socket a partir del  nombre de usuario en vector clientes
int getID(string name_user){
  int user;
  for(int i=0;i<clientes.size();i++){
    if(clientes[i].second[0] == name_user){
        user = clientes[i].first;
        return user;
    }
  }
  return -1;
}

//getSize: A partir del numero del tam de un array, devuelve el numero maximo que puede entrar en ese num  eg: 2 -> 99 | 1->9 | 3->999
int getSize(int s){
    string tam = "9";
    int res;
    for(int i=1;i<s;i++){tam += "9";}
    res = stoi(tam);
    return res;
}

//len: Devuelve el numero de caracteres que existe en un string
int len(char* w){
    string word(w);
    int count =0;
    while(word[count] != '\0'){
        count++;
    }
    return count;
}

//formatNumbers: Hace un formato a un numero ingresado, segun la cantidad de campos en un arreglo (rango)  eg: (2,5) -> 05  | (3,2) -> 002 | (3,15) -> 015
string formatNumbers(int range,int number){
    string numb = to_string(number);
    if(numb.size() < range){
        for(int i=0;i<range-numb.size()+1;i++){
            numb.insert(0,"0");
        }
    }
    return numb;
}

//search_user: busca un usuario dentro de los clientes conectados, si esta true caso contrario false
bool search_user(string usr_name){
  for(int i=0;i<clientes.size();i++){
    if(clientes[i].second[0] == usr_name){
      return true;
    }
  }
  return false;
}

//getDataFromPacket_Login: lee paquete login recibido del cliente 
// -msg: paquete de login con formato del protocolo 
// -data: vector que almacena [nombre,usuario]
// -socket: valor socket de quien envio paquete para login
bool getDataFromPacket_Login(string msg,vector<string> &data,int socket){
  string u,pssw;
  string tamU_str,tamPSSW_str;
  int tam_u,tam_pssw;

  if(msg[0] != 'l'){
    string msg_error;
    error e;
    e.set_msg("no coincide protocolo");
    msg_error = e.make_packet();
    write(socket,msg_error.c_str(),msg_error.size());
    return false;
  }

  msg.erase(0,1);

  tamU_str = msg.substr(0,2);
  tam_u = stoi(tamU_str);
  
  msg.erase(0,2);

  tamPSSW_str = msg.substr(0,2);
  tam_pssw = stoi(tamPSSW_str);
  
  msg.erase(0,2);

  u = msg.substr(0,tam_u);
  msg.erase(0,tam_u);

  pssw = msg.substr(0,tam_pssw);

  if(pssw != pass_general){
    string msg_error;
    error e;
    e.set_msg("Contrasenia no coincide");
    msg_error = e.make_packet();
    write(socket,msg_error.c_str(),msg_error.size());
    return false;
  }

  msg.erase(0,tam_pssw);

  data.push_back(u);
  data.push_back(pssw);

  if(msg.size() == 0){return true;}
  else{return false;}

}


/////////////
//STRUCTURES
/////////////

//okey: valida que se pudo hacer login
struct okey{
  char accion;
  char ok[3];

  okey(){
    accion = 'L';
    strcpy(ok,"ok");
  }

  string make_packet(){
    string packet = "";

    packet += accion;
    packet += ok;

    return packet;
  }
};

//list: listar los clientes conectados al servidor
struct list{
  char accion;
  char num_users[2];
  char tamanio_user_name[2];
  char* user_name;

  list(){
    accion = 'I';
  }

  string make_packet(){
    string packet = "";
    int num_clientes = clientes.size();
    size_t num_users_tam = sizeof(num_users)/sizeof(num_users[0]);
    string cant_clients = formatNumbers(num_users_tam,num_clientes);

    size_t tam_user_name;
    string tam_user_name_format;
    string user_names="";

    packet += accion;
    packet += cant_clients;

    for(int i=0;i<num_clientes;i++){
      tam_user_name = sizeof(tamanio_user_name)/sizeof(tamanio_user_name[0]);
      user_names += clientes[i].second[0];
      tam_user_name_format = formatNumbers(tam_user_name,clientes[i].second[0].size());
      packet += tam_user_name_format;
    }
    packet += user_names;

    return packet;
  }
};

//mensaje_user: enviar mensaje privados, de un cliente a otro cliente conectado
struct mensaje_user{
  char accion;
  char tamanio_msg[3];
  char tamanio_remitente[2];
  char* msg;
  char* remitente;

  mensaje_user(){
    accion = 'M';
    size_t tam_msg = sizeof(tamanio_msg)/sizeof(tamanio_msg[0]);
    size_t tam_dest = sizeof(tamanio_remitente)/sizeof(tamanio_remitente[0]);
    msg = new char[getSize(tam_msg)];
    remitente = new char[getSize(tam_dest)];
  }

  bool getDataFromPacket_MssgUser(string buff,int &socket_dest){

    string mssg,dest;
    string tammsg_str,tamdest_str;
    int tam_msg,tam_dest;

    if(buff[0] != 'm'){return false;}

    string mensg = "";
    mensg += buff;

    mensg.erase(0,1);

    tammsg_str = mensg.substr(0,3);
    tam_msg = stoi(tammsg_str);
  
    mensg.erase(0,3);

    tamdest_str = mensg.substr(0,2);
    tam_dest = stoi(tamdest_str);
    
    mensg.erase(0,2);

    mssg =  mensg.substr(0,tam_msg);
    mensg.erase(0,tam_msg);

    dest =  mensg.substr(0,tam_dest);
    mensg.erase(0,tam_dest);

    if(search_user(dest)){
      strcpy(msg,mssg.c_str());
      socket_dest = getID(dest);

      if(socket_dest == -1){
        return false;
      }

      return true;
    }else{
      return false;
    }

    if(mensg.size() == 0){return true;}
    else{return false;}

  }

  string make_packet(int socket_rem){
    strcpy(remitente,getUser(socket_rem).c_str());
    size_t tam_msg = sizeof(tamanio_msg)/sizeof(tamanio_msg[0]);
    size_t tam_rem = sizeof(tamanio_remitente)/sizeof(tamanio_remitente[0]);
    int numberChar_msg = len(msg);
    int numberChar_rem = len(remitente);

    string msg_size = formatNumbers(tam_msg,numberChar_msg);
    string rem_size = formatNumbers(tam_rem,numberChar_rem);

    string packet = "";

    packet += accion;
    packet += msg_size;
    packet += rem_size;
    packet += msg;
    packet += remitente;

    return packet;
  }

  bool sendMessage(int socket_dest, string message){
    for (int i = 0; i < clientes.size(); i++) {
      if(clientes[i].first == socket_dest){
        write(socket_dest, message.c_str(),message.size());
        return true;
      }
    }
    return false;
  }

};

//mensaje_all: enviar mensajes para todos, de un cliente para todos los clientes conectados
struct mensaje_all{
  char accion;
  char tamanio_msg[3];
  char tamanio_remitente[2];
  char* msg;
  char* remitente;

  mensaje_all(){
    accion = 'B';
    size_t tam_msg = sizeof(tamanio_msg)/sizeof(tamanio_msg[0]);
    size_t tam_rem = sizeof(tamanio_remitente)/sizeof(tamanio_remitente[0]);
    msg = new char[getSize(tam_msg)];
    remitente = new char[getSize(tam_rem)];
  }

  bool getDataFromPacket_MssgUser(string buff){

    string mssg,tammsg_str;
    int tam_msg;

    if(buff[0] != 'b'){return false;}

    string mensg = "";
    mensg += buff;

    mensg.erase(0,1);

    tammsg_str = mensg.substr(0,3);
    tam_msg = stoi(tammsg_str);
  
    mensg.erase(0,3);

    mssg =  mensg.substr(0,tam_msg);
    mensg.erase(0,tam_msg);

    strcpy(msg,mssg.c_str());

    if(mensg.size() == 0){return true;}
    else{return false;}

  }

  string make_packet(int socket_rem){
    strcpy(remitente,getUser(socket_rem).c_str());
    size_t tam_msg = sizeof(tamanio_msg)/sizeof(tamanio_msg[0]);
    size_t tam_rem = sizeof(tamanio_remitente)/sizeof(tamanio_remitente[0]);
    int numberChar_msg = len(msg);
    int numberChar_rem = len(remitente);

    string msg_size = formatNumbers(tam_msg,numberChar_msg);
    string rem_size = formatNumbers(tam_rem,numberChar_rem);

    string packet = "";

    packet += accion;
    packet += msg_size;
    packet += rem_size;
    packet += msg;
    packet += remitente;

    return packet;
  }

  bool sendMessage(string message){

    for (int i = 0; i < clientes.size(); i++) {
      if(clientes[i].first != getID(remitente)){
        write(clientes[i].first, message.c_str(),message.size());
      }
    }return true;

  }

};

//upload_file: subir un archivo al servidor
struct uploadfile{
  char accion;
  char tamanio_file_name[3];
  char tamanio_file_data[10];
  char tamanio_remitente[2];
  char* file_name;
  char* file_data;
  char* remitente;

  uploadfile(){
    accion = 'U';
    size_t tam_fn = sizeof(tamanio_file_name)/sizeof(tamanio_file_name[0]);
    size_t tam_fd = sizeof(tamanio_file_data)/sizeof(tamanio_file_data[0]);
    size_t tam_re = sizeof(tamanio_remitente)/sizeof(tamanio_remitente[0]);
    file_name = new char[getSize(tam_fn)];
    file_data = new char[getSize(tam_fd)];
    remitente = new char[getSize(tam_re)];
  }



};

//file_AN: que un archivo recibido por un cliente pueda ser aceptado o rechazado
struct file_AN{
  char accion;
  char tamanio_user_name[2];
  char* user_name;

  file_AN(){
    accion = 'F';
    size_t tam_u = sizeof(tamanio_user_name)/sizeof(tamanio_user_name[0]);
    user_name = new char[getSize(tam_u)];
  }

};

//salir: que un cliente se desconecte del server
struct salir{
  char accion;

  salir(){
    accion = 'X';
  }

  string make_packet(){
    string packet = "";

    packet += accion;
    return packet;
  }

  void deleteSocket(int id){
    int pos=0;
    for(;pos<clientes.size();pos++){
      if(clientes[pos].first == id){break;}
    }

    shutdown(id, SHUT_RDWR);
    //close(id);
    clientes.erase(clientes.begin()+pos);
  }
};


////////////////////////////////////////////
////////////////////////////////////////////

//ESTABLECE CONEXIONES INICIALES
void Connection( int &SocketFD , int port)
{
    struct sockaddr_in stSockAddr;
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if(-1 == SocketFD)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }
    
    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
   
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;
 
    if(-1 == bind(SocketFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
    {
      perror("error bind failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    
    if(-1 == listen(SocketFD, 10))
    {
      perror("error listen failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
   
}

//ESCUCHA CONEXIONES DE CLIENTES
void listenClient(int ConnectFD)
{
    //cout<<"open funcion Listen-server"<<endl;
    char buffer[256];
    int n;

    string message;

    bzero(buffer,256); 
    n = read(ConnectFD,buffer,255);
    if (n < 0) perror("ERROR reading from socket");

    vector<string> datos_login;
    bool validate_login= getDataFromPacket_Login(buffer,datos_login,ConnectFD);

    if(validate_login){

      auto par = make_pair(ConnectFD,datos_login);
      clientes.push_back(par);
      
      okey k;
      string msg = k.make_packet();
      write(ConnectFD, msg.c_str(),msg.size());

      //regex r("(\\w|\\d|\\s1\\W)");
      regex r("\\d");

      while(1){

        bzero(buffer,256); 
        n = read(ConnectFD,buffer,255);
        if (n < 0){
           //perror("ERROR reading from socket");
           break;
        }
        //cout<<"buffer-read-server: "<<buffer<<endl;

         string user = getUser(ConnectFD);
         string character = "";
         int tam_msg = len(buffer);
         character += buffer[1];
        ///analizador de segundo caracter

        if(buffer[0] == 'i' && tam_msg == 1){
          list users;
          msg = users.make_packet();
          write(ConnectFD,msg.c_str(),msg.size());
          bzero(buffer,256);
          msg = "";
        }

        else if(buffer[0] == 'x' && tam_msg == 1){
          salir e;
          msg = e.make_packet();
          write(ConnectFD,msg.c_str(),msg.size());
          bzero(buffer,256);
          msg = "";
          e.deleteSocket(ConnectFD);
          close(ConnectFD);
          if(clientes.size()){
            ConnectFD = clientes[0].first;
          }
        }

        else if(buffer[0] == 'm' && regex_match(character,r)){
          mensaje_user ms;
          int socket_f = ConnectFD;
          int socket_d;
          ms.getDataFromPacket_MssgUser(buffer,socket_d);
          msg = ms.make_packet(socket_f);
          ms.sendMessage(socket_d,msg);
          bzero(buffer,256);
          msg = "";
        }

        else if(buffer[0] == 'b' && regex_match(character,r)){
          mensaje_all ms;
          int socket_f = ConnectFD;
          ms.getDataFromPacket_MssgUser(buffer);
          msg = ms.make_packet(socket_f);
          ms.sendMessage(msg);
          bzero(buffer,256);
          msg = "";
        }

        else{
          cout<<"Ingrese un comando"<<endl;
          bzero(buffer,256);
          msg = "";
          //cout<<user<<": "<<buffer<<endl;
          //cout<<"Type a message: ";
          //message= user + ": " + buffer;
          //getline(cin,message);
          //Broadcast(ConnectFD, message);
        }
      }
    }
    else{
      for(int i = 0; i < clientes.size(); ++i) 
      {
        if (clientes[i].first == ConnectFD) 
        {
          shutdown(ConnectFD, SHUT_RDWR);
          close(ConnectFD);
          clientes.erase(clientes.begin() + i);
          break;
        }
      }
    }
    
    /*
    cout<<"borrar server"<<endl;
    for(int i = 0; i < clientes.size(); ++i) 
    {
      if (clientes[i].first == ConnectFD) 
      {
        shutdown(ConnectFD, SHUT_RDWR);
        close(ConnectFD);
        clientes.erase(clientes.begin() + i);
      }
    }
    */
}

int main(void)
{
  struct sockaddr_in stSockAddr;
  int SocketFD;
  char buffer[256];
  int n;
  cout<<"Beginning the server"<<endl;
  Connection( SocketFD, 45000);
  
  cout<<"Connection initialized"<<endl;
  
  while(1)
  {
    int ConnectFD = accept(SocketFD, NULL, NULL);

    if(0 > ConnectFD)
    {
      perror("error accept failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    //clients.push_back(ConnectFD);

    thread (listenClient, ConnectFD).detach();

    
  }

  close(SocketFD);
  cout<<"Bye"<<endl;
  return 0;
}

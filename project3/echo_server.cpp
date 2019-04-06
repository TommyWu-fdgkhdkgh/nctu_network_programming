#include <array>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

namespace bp = boost::process;

using namespace std;
using namespace boost::asio;

extern char **environ;


//https://stackoverflow.com/questions/717239/io-service-why-and-how-is-it-used
//io_service:他先用select去檢查每個fd是不是readable or writable 。 檢查完fd後，開始遍尋所有fd。io_service會幫我們維護所有fd的意義（read ? write?），readable的fd就call read function，writable的fd就call write 所需要的function。
//當沒有新的call back出現的時候，這個io_service就會自己消失。
io_service global_io_service;

//Q:這串程式碼是怎麼做到每個client的字串互不影響的？
//A:每個session都有自己的buffer

//argv要填，不然結果會出不來

//Q:該怎麼對付會延遲輸入的cgi??
//這樣勢必需要一個async的機制
//我發覺就算我發展了一個async的機制，可能還是沒辦法，因為會不停的等到'\0'的出現

struct OtherWork {
    using clock = std::chrono::high_resolution_clock;

    OtherWork(boost::asio::io_context& io) : timer(io) { }

    void start() {
        timer.expires_at(clock::time_point::max());
        loop();
    }

    void stop() {
        timer.expires_at(clock::time_point::min());
    }

  private:
    void loop() {
        if (timer.expires_at() == clock::time_point::min()) {
            std::cout << "(done)" << std::endl;
            return;
        }

        timer.expires_from_now(std::chrono::milliseconds(2793));
        timer.async_wait([=](boost::system::error_code ec) {
            if (!ec) {
                std::cout << "(other work in progress)" << std::endl;
                start();
            } else {
                std::cout << "(" << ec.message() << ")" << std::endl;
            }
        });
    }
    boost::asio::high_resolution_timer timer;
};


class EchoSession : public enable_shared_from_this<EchoSession> {
 private:
  enum { max_length = 1000 };

  //for socket connected with client
  ip::tcp::socket _socket;
  array<char, max_length> _data;
  boost::asio::signal_set signal_;

  int number;
  /*
   * index 0 : cgi_name
   * index 1~3 : parameters for first server
   * */
  char parameters[16][50];
  char temp_header[500];
  /*char cgi_name[30];
  char h0[30],p0[30],f0[30];
  char h1[30],p1[30],f1[30];
  char h2[30],p2[30],f2[30];
  char h3[30],p3[30],f3[30];
  char h4[30],p4[30],f4[30]; */

  int header_flag; //1 send http status header(ex:200 OK)
                   //0 don't send anything

 public:
  //EchoSession(ip::tcp::socket socket) : _socket(move(socket)){}
  EchoSession(ip::tcp::socket socket) : _socket(move(socket)),
	                                signal_(global_io_service,SIGCHLD){
    wait_for_signal();					
 					
  }

  //在這個echo server裡，
  void start() {
    int number=0;


    //紀錄是不是第一筆送出（第一筆送出需要多加一個header）
    header_flag=1;

    //第一個讀到的是client傳來的request，寫個程式來parse header
    do_read_with_head_from_client();

  }

 private:
  void wait_for_signal()
  {
    signal_.async_wait(
      [this](boost::system::error_code /*ec*/, int /*signo*/)
      {
        // Only the parent process should check for this signal. We can
        // determine whether we are in the parent by checking if the acceptor
        // is still open.
        //if (acceptor_.is_open())
        //{
          // Reap completed child processes so that we don't end up with
          // zombies.
          int status = 0;
          while (waitpid(-1, &status, WNOHANG) > 0) {}

          wait_for_signal();
        //}
      });
  }


  void fork_and_exe_cgi(){
    //turn _socket to fd
    //Q:socket fd 是不是該關掉？
    int socketfd = _socket.native_handle();

    printf("parameters[0] %s\n",parameters[0]);
    char p1[50];
    memcpy(p1,parameters[0],strlen(parameters[0])+1);
    printf("p1:%s\n",p1);

    global_io_service.notify_fork(boost::asio::io_context::fork_prepare);
    if (fork() == 0)
    {
      //child
      global_io_service.notify_fork(boost::asio::io_context::fork_child);

      auto self(shared_from_this());

      //set env that cgi need 
      set_env();
      

      //send OK head
      array<char,20> okhead= {"HTTP/1.1 200 OK\r\n\0"};
      int index=0;
      while(1){
        _data[index]=okhead[index];
        if(okhead[index]!='\0'){
	  index++;
	}else{
          break;
	}	
      }
      //_socket.async_send(
      _socket.async_write_some(
        buffer(_data, index),
        [this,self](boost::system::error_code ec, std::size_t length){
      });

     
      //dup stdout to socketfd 
      dup2(socketfd,1);
      //dup2(socketfd,2);   
     
      char *argvs[2];
      char p1[50];

      //執行檔名稱後面多一個空格就會炸開
      //memcpy(p1,"./panel.cgi \x00",strlen("./panel.cgi \x00"));
      memcpy(p1,parameters[0],strlen(parameters[0])+1);
      argvs[0]=p1;
      argvs[1]=NULL;

      execve(argvs[0],argvs,environ);


    }
    else
    {
      //parent
      global_io_service.notify_fork(boost::asio::io_context::fork_parent);

    }

  }

  void set_env(){
    /*
       *REQUEST_METHOD:GET
       *REQUEST_URI:/console.cgi?tid=222
       *QUERY_STRING:tid=222&dd=111
       *SERVER_PROTOCOL:HTTP/1.1
       *HTTP_HOST:npbsd3.cs.nctu.edu.tw:55555
       *SERVER_ADDR:140.113.235.223
       *SERVER_PORT:55555
       *REMOTE_ADDR:client端的host名稱
       *REMOTE_PORT:client端的port
    */
    int index=0;
    int tindex=0;
    int state;
    int flag=0;
    char tempstr[500];

    //REQUEST_METHOD
    while(1){
      if(_data[index]!=' '){
        tempstr[index]=_data[index];
      }else
        break;

      index++;
    }
    tempstr[index]='\x00';
    setenv("REQUEST_METHOD",tempstr,1); 
    printf("REQUEST_METHOD:%s\n",tempstr);

    //REQUEST_URI
    index=0;
    state=0;
    tindex=0;
    while(1){
      if(state==0 && _data[index]=='/'){
        state=1;
      } 

      if(state==1 && _data[index]==' '){
        break;
      }

      if(state==1){
        tempstr[tindex++]=_data[index];
      }
      index++;
    }
    tempstr[tindex]='\x00';
    setenv("REQUEST_URI",tempstr,1); 
    printf("REQUEST_URI:%s\n",tempstr);

    //QUERY_STRING
    index=0;
    state=0;
    tindex=0;
    flag=0;    
    while(1){
      if(state==0 && _data[index]=='?'){
        state=1;
	index++;
      }

      if(state==0 && _data[index]=='\n'){
	flag=1;
        break;
      }

      if(state==1 && _data[index]==' '){
        break;
      }

      if(state==1){
        tempstr[tindex++]=_data[index];
      }

      index++;

    }
    tempstr[tindex]='\x00';
    if(flag==1){
      printf("no query_string!\n");
    }else{
      setenv("QUERY_STRING",tempstr,1); 
      printf("QUERY_STRING:%s\n",tempstr);
    }

    
    //SERVER_PROTOCOL:HTTP/1.1
    index=0;
    tindex=0;
    state=0;
    while(1){
      if(state==0 && _data[index]==' '){
        state++;
	index++;
	continue;
      }

      if(state==1 && _data[index]==' '){
        state++;
	index++;
	continue;
      }
      
      if(state==2 && _data[index]==' '){
        break;
      }

      if(state==2){
        tempstr[tindex++]=_data[index];  
      }

      index++;
    }

    tempstr[tindex]='\x00';
    setenv("SERVER_PROTOCOL",tempstr,1); 
    printf("SERVER_PROTOCOL:%s\n",tempstr); 

    
    //HTTP_HOST:npbsd3.cs.nctu.edu.tw:55555
    index=0;
    tindex=0;
    state=0;
    while(1){
      if(state==0 && _data[index]=='\n'){
        state++;
	index++;
      }

      if(state==1 && _data[index]==' '){
        state++;
	index++;
      }

      if(state==2 && _data[index]=='\n'){
        break;
      }

      if(state==2){
        tempstr[tindex++]=_data[index];
      } 
      
      index++;
    }

    tempstr[tindex]='\x00';
    setenv("HTTP_HOST",tempstr,1); 
    printf("HTTP_HOST:%s\n",tempstr);

    
    //SERVER_ADDR:140.113.235.223
    //http://lang.idv.tw/doku.php/program/c/linux%E7%B7%A8%E7%A8%8B%E7%8D%B2%E5%8F%96%E6%9C%AC%E6%A9%9Fip%E5%9C%B0%E5%9D%80
    //https://blog.csdn.net/huang_xw/article/details/8502895
    ip::tcp::endpoint local_e = _socket.local_endpoint(); 
    string temp = local_e.address().to_string();
    int len = temp.length();
    for(index=0;index<len;index++){
      tempstr[index]=temp[index]; 
    }    
    tempstr[index]='\x00';
   
    setenv("SERVER_ADDR",tempstr,1); 
    printf("SERVER_ADDR:%s\n",tempstr);
    

    //SERVER_PORT:55555
    index=0;
    tindex=0;
    state=0;
    while(1){
      if(state==0 && _data[index]=='\n'){
        state++;
	index++;
      }

      if(state==1 && _data[index]==' '){
        state++;
	index++;
      }

      if(state==2 && _data[index]==':'){
        state++;
	index++;
      }

      if(state==3 && _data[index]=='\n'){
        break;
      }

      if(state==3){
        tempstr[tindex++]=_data[index];
      } 
      
      index++;
    }

    tempstr[tindex]='\x00';
    setenv("SERVER_PORT",tempstr,1); 
    printf("SERVER_PORT:%s\n",tempstr);
    
    //REMOTE_ADDR:client端的host名稱
    ip::tcp::endpoint remote_e = _socket.remote_endpoint(); 
    temp = remote_e.address().to_string();
    len = temp.length();
    for(index=0;index<len;index++){
      tempstr[index]=temp[index]; 
    }    
    tempstr[index]='\x00';

    setenv("REMOTE_ADDR",tempstr,1); 
    printf("REMOTE_ADDR:%s\n",tempstr);

    //REMOTE_PORT:client端的port
    short remote_port = remote_e.port();
    sprintf(tempstr,"%hu",&remote_port);
    setenv("REMOTE_PORT",tempstr,1); 
    printf("REMOTE_PORT:%s\n",tempstr); 
    

  }

  void parse_header(){
    //initialize parameters
    for(int i=0;i<16;i++){
      for(int j=0;j<50;j++){
        parameters[i][j]='\x00';
      }
    }

    //parse header
    int index=0;
    while(1){
      temp_header[index]=_data[index];
      if(_data[index]=='\n'){
	temp_header[index]='\x00';
        break;
      }
      index++;
    }

   
    //cgi_name
    int state=0;
    int tindex=0;
    index=0;
  
    parameters[0][tindex++]='.';

    while(1){

      if(temp_header[index]=='/'){
        state=1;
      }else if(state==1 && (temp_header[index]=='?' || temp_header[index]==' ')){
        break;
      }

      if(state==1){
        parameters[0][tindex++]=temp_header[index]; 
      }
      index++;
    }
    parameters[0][tindex]='\x00';

    if(temp_header[index-1]==' '){
      //沒有任何參數，跳離function
      return;
    }


    ///console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0=55555&f0=1.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4= HTTP/1.1
    //hn,pn,fn
    state=0;
    tindex=0;
    for(int i=0;i<15;i++){
      printf("hi head \n");
      state=0;
      tindex=0; 
      while(1){

        if(temp_header[index]=='='){
          state=1;
	  index++;
	}
	
	if(temp_header[index]=='&' || temp_header[index]==' '){
          index++;
          break;
	}

        if(state==1){
          parameters[i+1][tindex++] = temp_header[index];
	}

        index++;
      } 

      if(temp_header[index-1]==' '){
        //no more parameters
	break;
      }

      parameters[i+1][tindex]='\x00';
    }
   
    for(int i=0;i<16;i++){ 
      printf("parameters[%d] %s\n",i,parameters[i]);
    }

    printf("\n\nnumber:%d\n",number++);
    printf("\n\n header now  %s\n\n",temp_header);  
    printf("\n\n cgi name %s \n\n",parameters[0]);

  }

  void do_read_with_head_from_client(){
    //讀取來自客戶端的訊息
    auto self(shared_from_this());
    _socket.async_read_some(
      buffer(_data, max_length),
      [this, self](boost::system::error_code ec, std::size_t length) {

        //if (!ec) do_write(length);
	cout<<"_data:"<<endl;

        for(int i=0;;i++){
          if(_data[i]!='\0'){
            printf("%c",_data[i]);
	  }else{
	    printf("%c",_data[i]);
	    break;
	  }
	}

	parse_header();

        fork_and_exe_cgi();

    });
   
    

  }
};

class EchoServer {
 private:
  ip::tcp::acceptor _acceptor;
  ip::tcp::socket _socket;

 public:
  EchoServer(short port)
        //這裡會把接受新連線用的socket fd丟到io_service裡面維護（io_service裡面的select）
      : _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
        _socket(global_io_service)
        //connection_manager()       
	{

    //執行第一個異步的wait for accept
    do_accept();
  }

 private:
  void do_accept() {

    //這裡會把接受新連線用的socket fd丟到io_service裡面維護（io_service裡面的selecti）
    //並且也把call back function也放到io_service裡
    _acceptor.async_accept(_socket, [this](boost::system::error_code ec) {

      //拿到這個socket後，開始利用這個socket進行連線
      //因為server這個object不會再用到socket，所以用move來完全的轉移對於socket這個物件的權限
      //可以研究一下move的機制
      if (!ec) make_shared<EchoSession>(move(_socket))->start();

      do_accept();
    });
  }
};


int main(int argc, char* const argv[]) {
  if (argc != 2) {
    std::cerr << "Usage:" << argv[0] << " [port]" << endl;
    return 1;
  }
  

  try {
    short port = atoi(argv[1]);
    EchoServer server(port);
    global_io_service.run();
    //cgi_io_service.run();
  } catch (exception& e) {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

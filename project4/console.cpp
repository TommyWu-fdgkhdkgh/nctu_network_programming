#include <cstring>
#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <utility>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>


using boost::asio::ip::tcp;

//enum { max_length = 1024 };
//std::array<char , max_length> _data;
boost::asio::io_context io_context;
//tcp::socket s(io_context);
//tcp::resolver resolver(io_context);

void print_initial_html();
void parse_env();

class clientSession : public std::enable_shared_from_this<clientSession> {
  private:
    enum { max_length=5000 };
    tcp::socket s;
    tcp::resolver resolver;
    std::array<char , max_length> _data;
    char *filename;
    std::ifstream file;
    int client_index;
    boost::asio::deadline_timer timer;

    void do_read(int flag){

      auto self(shared_from_this());
      //std::cout<<"start to read!"<<std::endl;

      s.async_read_some(
        boost::asio::buffer(_data, max_length),
        [this, self ,flag](boost::system::error_code ec, std::size_t length) {
          if (!ec){

            _data[length]='\x00';

            std::string tempstr;	   
	    int active_flag=0;//區分要不要do_write 
            tempstr.clear();
	    
	    /*int index=0;
	    while(1){
	      tempstr+=_data[index]; 
              if(_data[index]=='\x00')
	        break;
	    
              index++;
            }
	    std::cout<<tempstr<<std::endl;
	    tempstr.clear();*/
            

            //if(_data[0]=='%' && _data[1]==' '){
            if(_data[0]=='%' && _data[1]==' '){
              //std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<"type 1"<<"\";</script>" << std::endl;
              active_flag=1;
              std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<"% "<<"\";</script>" << std::endl;

	    }else{
              //std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<"type 2"<<"\";</script>" << std::endl;
              int index=0;

              //std::cout<<_data<<std::endl;	      
  
              while(1){
	        if(_data[index]=='\n'){
	          if(tempstr[0]=='%' && tempstr[1]==' '){
                    std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<"% "<<"\";</script>" << std::endl;
		    active_flag=1;
		    tempstr.clear();
		  }else{
                    tempstr+="&NewLine;";
                    std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<tempstr<<"\";</script>" << std::endl;
		    std::cout << tempstr << std::endl;
		    tempstr.clear();
		  }
	        }else if(_data[index]!='\x00'){
	          if(_data[index]=='<'){
		    //要用html encode才可以在網頁上顯示html tag
                    tempstr+="&lt;";
		  }else if(_data[index]=='>'){
                    tempstr+="&gt;";
		  }else if(_data[index]=='\r' || _data[index]=='\x08'){
	            //這一步卡了我好久= = 
		  }else if(_data[index]=='\"'){
		    tempstr+="&#x22;";  
		  }else if(_data[index]=='\''){
		    tempstr+="&#x27;";
		  }else{
                    tempstr+=_data[index];
		  }
	        }
	        //printf("%d bytes: %c : %hhd\n",index,_data[index],_data[index]);
                if(_data[index]=='\x00'){
                  std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<tempstr<<"\";</script>" << std::endl;
		  tempstr.clear();
                  break;
                }
                index++;
              }
	    }


	    if(active_flag==1){
              timer.expires_from_now(boost::posix_time::seconds(1));
              timer.async_wait([this,self](boost::system::error_code ec){
	        if(!ec){
	          do_write();
		}		      
	      });
	    }
	    do_read(1);
	    //<script>document.getElementById('s0').innerHTML += '% ';</script>
	    //0 if we want to read %
	    
            /*if(flag==1){
              do_write();
	    }else if(flag==0){
              do_read(1);
	    }*/
          }else{
            //std::cout<<"read ec :"<<ec<<std::endl;
	  }
	}
      );
    }

    void do_write(){

      auto self(shared_from_this());

      //std::cout<<"getline"<<std::endl;
      char request[max_length];
      std::string line;
      std::string tempstr;
      size_t len=0;
      ssize_t read;


      if(std::getline(file,tempstr)){
	int index=0;
	for(index=0;index<tempstr.length();index++){
          if(tempstr[index]=='<'){
            //要用html encode才可以在網頁上顯示html tag
            line+="&lt;";
          }else if(tempstr[index]=='>'){
            line+="&gt;";
          }else if(tempstr[index]=='\r' || tempstr[index]=='\x08' || tempstr[index]=='\n'){
	    //這一步卡了我好久= = 
          }else if(tempstr[index]=='\"'){
            line+="&#x22;";  
          }else if(tempstr[index]=='\''){
            line+="&#x27;";
          }else{
            line+=tempstr[index];
	  }
	}
	
        std::cout<<"<script>document.getElementById('s"<<client_index<<"').innerHTML += '<b>"<<line<<"&NewLine;</b>';</script>" << std::endl;

	tempstr = tempstr + "\n";

        s.async_send(
          boost::asio::buffer(tempstr,tempstr.length()),
          [this, self,request](boost::system::error_code ec, std::size_t length/* length */) {
            if (!ec){
	    }
          }
        );
      }else{
        //do nothing?
      }
    }

  public:
    clientSession(char *name , char *port , char *filename ,int client_index , char *sh , char *sp , int socks_exist):
	                                  s(io_context),
	                                  resolver(io_context),
					  filename(filename),
					  timer(io_context),
	                                  client_index(client_index){
 
      if(socks_exist){

        printf("sppppppppppppppp :%s\n",sp);
        printf("shhhhhhhhhhhhhhh :%s\n",sh);

        int portnumber = atoi(port);

        struct hostent *a =  gethostbyname(name);

        unsigned char socks_request[8];
        socks_request[4]=a->h_addr_list[0][0];
        socks_request[5]=a->h_addr_list[0][1];
        socks_request[6]=a->h_addr_list[0][2];
        socks_request[7]=a->h_addr_list[0][3];
      
        socks_request[0]=4;
        socks_request[1]=1;

        socks_request[2]= portnumber>>8;
        socks_request[3]= portnumber%(1<<8);

        for(int i=0;i<8;i++){
          printf("send [%d] : %hhu",i,socks_request[i]);
	}

        boost::asio::connect(s, resolver.resolve(sh,sp));
        std::string tempFilename("./test_case/");
        int tindex=0;
        int len=strlen(filename);
        for(tindex=0;tindex<len;tindex++){
  	  tempFilename+=filename[tindex];
        }
      
        //std::cout<<"name:"<<name<<std::endl;
        //std::cout<<"tempFilename:"<<tempFilename<<std::endl;
        std::cout<<std::endl;
        file.open(tempFilename,std::ifstream::in);

        write(s.native_handle(),socks_request,8);	

      }else{
        boost::asio::connect(s, resolver.resolve(name, port));
        std::string tempFilename("./test_case/");
        int tindex=0;
        int len=strlen(filename);
        for(tindex=0;tindex<len;tindex++){
  	  tempFilename+=filename[tindex];
        }
      
        //std::cout<<"name:"<<name<<std::endl;
        //std::cout<<"tempFilename:"<<tempFilename<<std::endl;
        std::cout<<std::endl;
        file.open(tempFilename,std::ifstream::in);
      }
    }

    void start(){
      //std::cout<<"here we come!"<<std::endl;
      do_read(1);
    }


};


char *query_string;
char parameters[17][50];
int number_of_console=0;

int main(int argc, char* argv[])
{

  enum { max_length = 1024 };

  try
  {
    /*if (argc != 3)
    {
      std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
      return 1;
    }*/
   
    number_of_console=0; 

    query_string = getenv("QUERY_STRING"); 


    //parse env to local variable
    parse_env();




    //parse_env(); // just for test
    for(int i=0;i<15;i++){
      if((i%3)==0){
        if(parameters[i][0]!='\x00'){
          number_of_console++;
	}	
      }
    }

    //print html
    print_initial_html(); 


    //document.getElementById('s0').innerHTML += '<b>ls&NewLine;</b>';

    /*for(int i=0;i<15;i++){
      printf("parameters[%d] %s\n",i,parameters[i]);
    }*/

    //std::cout<<"number:"<<number_of_console<<std::endl;

    //std::cout<<"ggler"<<std::endl;
    //new a client session
    int socks_exist = 0;
    if(parameters[15][0]!=0){
      socks_exist=1;
    }

    for(int i=0;i<15;i+=3){
      if(parameters[i][0]!='\x00'){
        std::make_shared<clientSession>(parameters[i],parameters[i+1],parameters[i+2],i/3,parameters[15],parameters[16],socks_exist)->start();
      }
      //std::cout<<parameters[i*3]<<std::endl<<parameters[1+i*3]<<std::endl<<parameters[2+i*3]<<std::endl;
    }

    io_context.run();

    //std::cout<<"ggler"<<std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

void parse_env(){

  //initialize parameters
  for(int i=0;i<17;i++){
    for(int j=0;j<50;j++){
      parameters[i][j]='\x00';
    }
  }

  //std::cout<<query_string<<std::endl;

  //h0=nplinux1.cs.nctu.edu.tw&p0=55555&f0=1.txt&h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=
  int len = strlen(query_string);
  int state=0;
  int tindex=0;
  int index=0;
  for(int i=0;i<17;i++){
    //printf("hi head \n");
    state=0;
    tindex=0;
    while(1){

      if(index==len){
	//no more char
        break;
      }

      if(query_string[index]=='='){
        state=1;
        index++;
      }

      if(query_string[index]=='&'){
        index++;
        break;
      }

      if(state==1){
        parameters[i][tindex++] = query_string[index];
      }

      index++;
    }

    parameters[i][tindex]='\x00';

    if(index==len){
      //no more parameters
      break;
    }

  }


}

void print_initial_html(){
  /* [Required] HTTP Header */
  std::cout << "Content-type: text/html" << std::endl << std::endl;

  std::cout << "<!DOCTYPE html>" << std::endl;
  std::cout << "<html lang=\"en\">" << std::endl;
  std::cout << "  <head>" << std::endl;
  std::cout << "    <meta charset=\"UTF-8\" />" << std::endl;
  std::cout << "    <title>NP Project "<<number_of_console<<" Console</title>" << std::endl;
  std::cout << "    <link" << std::endl;
  std::cout << "      rel=\"stylesheet\"" << std::endl;
  std::cout << "      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"" << std::endl;
  std::cout << "      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"" << std::endl;
  std::cout << "      crossorigin=\"anonymous\"" << std::endl;
  std::cout << "    />" << std::endl;
  std::cout << "    <link" << std::endl;
  std::cout << "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"" << std::endl;
  std::cout << "      rel=\"stylesheet\" << std::endl;" << std::endl;
  std::cout << "    />" << std::endl;
  std::cout << "    <link" << std::endl;
  std::cout << "      rel=\"icon\"" << std::endl;
  std::cout << "      type=\"image/png\"" << std::endl;
  std::cout << "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"" << std::endl;
  std::cout << "    />" << std::endl;
  std::cout << "    <style>" << std::endl;
  std::cout << "      * {" << std::endl;
  std::cout << "        font-family: 'Source Code Pro' , monospace;" << std::endl;
  std::cout << "        font-size: 1rem !important;" << std::endl;
  std::cout << "      }" << std::endl;
  std::cout << "      body  {" << std::endl;
  std::cout << "            background-color: #212529;" << std::endl;
  std::cout << "      }" << std::endl;
  std::cout << "      pre  {" << std::endl;
  std::cout << "        color: #cccccc;" << std::endl;
  std::cout << "      }" << std::endl;
  std::cout << "      b  {" << std::endl;
  std::cout << "        color: #ffffff;" << std::endl;
  std::cout << "      }" << std::endl;
  std::cout << "    </style>" << std::endl;
  std::cout << "  </head>" << std::endl;
  std::cout << "  <body>" << std::endl;
  std::cout << "    <table class=\"table table-dark table-bordered\">" << std::endl;
  std::cout << "      <thead>" << std::endl;
  std::cout << "        <tr>" << std::endl;

  for(int i=0;i<15;i+=3){
    if(parameters[i][0]!='\x00'){
      std::cout << "          <th scope=\"col\">"<<parameters[i]<<":"<<parameters[i+1]<<"</th>" << std::endl;
    }
  }
  std::cout << "        </tr>" << std::endl;
  std::cout << "      </thead>" << std::endl;
  std::cout << "      <tbody>" << std::endl;
  std::cout << "        <tr>" << std::endl;
  for(int i=0;i<15;i+=3){ 
    if(parameters[i][0]!='\x00'){
      std::cout << "          <td><pre id =\"s"<<i/3<<"\" class=\"mb-0\"></pre></td>" << std::endl;
    }
  }
  std::cout << "        </tr>" << std::endl;
  std::cout << "      </tbody>" << std::endl;
  std::cout << "    </table>" << std::endl;
  std::cout << "  </body>" << std::endl;
  std::cout << "</html>" << std::endl;


}

/*
  need to do:
    every session open file

    parse env to local variable

core dump:
  可能是記憶體用太兇

*/

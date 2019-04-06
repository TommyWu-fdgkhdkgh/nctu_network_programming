#include <array>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>

using namespace std;
using namespace boost::asio;

io_service global_io_service;




class EchoSession : public enable_shared_from_this<EchoSession> {
private:
	enum { max_length = 50000 };
	array<char, max_length> _data;
	char parameters[16][50];
	char temp_header[500];

/***********inner class**************/
	class clientSession : public std::enable_shared_from_this<clientSession> {
	private:
		enum { max_length = 5000 };
		boost::asio::ip::tcp::socket s;
		boost::asio::ip::tcp::resolver resolver;
		std::array<char, max_length> _data;
		std::ifstream file;
		char *filename;
		char in_class_parameters[16][50];
		int client_index;
		//EchoSession &parent;

	public:
		std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
		std::string resultstr; //回傳給EchoSession，送給瀏覽器
		//boost::asio::ip::tcp::socket browser_s;

		clientSession(char *name, char *port, char *filename, int client_index, std::shared_ptr<boost::asio::ip::tcp::socket> _socket/*, boost::asio::ip::tcp::socket browser_s*/) :
			client_index(client_index),
			s(global_io_service),
			//browser_s(move(browser_s)),
			//parent(parent),
			filename(filename),
			_socket(_socket),
			resolver(global_io_service)
		{
			boost::asio::connect(s, resolver.resolve(name, port));
			std::string tempFilename("./test_case/");
			int tindex = 0;
			size_t len = strlen(filename);
			for (tindex = 0; tindex<len; tindex++) {
				tempFilename += filename[tindex];
			}

			file.open(tempFilename, std::ifstream::in);

			/*for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 50; j++) {
			in_class_parameters[i][j] = parameters[i][j];
			}
			}*/




		}
		~clientSession() {
			file.close();
		}

		void start() {
			printf("start!\n");

			do_read();
		}

		void do_read() {
			auto self(shared_from_this());

			s.async_read_some(
				boost::asio::buffer(_data, max_length),
				[this, self](boost::system::error_code ec, std::size_t length) {
				if (!ec) {


					_data[length] = '\x00';

					std::string tempstr;
					std::string sendstr;
					int active_flag = 0;//區分要不要do_write 
					tempstr.clear();
					sendstr.clear();

					array<char, 3000> tempdata;


					if (_data[0] == '%' && _data[1] == ' ') {
						//std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<"type 1"<<"\";</script>" << std::endl;
						//std::cout << "<script>document.getElementById('s" << client_index << "').innerHTML+=\"" << "type 1" << "\";</script>" << std::endl;

						active_flag = 1;
						//std::cout << "<script>document.getElementById('s" << client_index << "').innerHTML+=\"" << "% " << "\";</script>" << std::endl;
						sendstr = "<script>document.getElementById('s" + to_string(client_index) + "').innerHTML+=\"" + "% " + "\";</script>\n";

						int i = 0;
						for (i = 0; i < sendstr.length(); i++) {
							tempdata[i] = sendstr[i];
						}


						_socket->async_send(
							boost::asio::buffer(tempdata, i),
							[this](boost::system::error_code ec, std::size_t length/* length */) {
							if (!ec) {
								printf("after send to browser\n");
							}
						}
						);


						tempstr.clear();
						sendstr.clear();

					}
					else {
						//std::cout <<"<script>document.getElementById('s"<<client_index<<"').innerHTML+=\""<<"type 2"<<"\";</script>" << std::endl;
						int index = 0;

						//std::cout<<_data<<std::endl;	      

						while (1) {
							if (_data[index] == '\n') {
								if (tempstr[0] == '%' && tempstr[1] == ' ') {

									sendstr = "<script>document.getElementById('s" + to_string(client_index) + "').innerHTML+=\"" + "% " + "\";</script>\n";

									int i = 0;
									for (i = 0; i < sendstr.length(); i++) {
										tempdata[i] = sendstr[i];
									}


									_socket->async_send(
										boost::asio::buffer(tempdata, i),
										[this](boost::system::error_code ec, std::size_t length/* length */) {
										if (!ec) {
											printf("after send to browser\n");
										}
									}
									);

									sendstr.clear();
									tempstr.clear();
									active_flag = 1;

								}
								else {
									tempstr += "&NewLine;";
									//std::cout << "<script>document.getElementById('s" << client_index << "').innerHTML+=\"" << tempstr << "\";</script>" << std::endl;
									sendstr = "<script>document.getElementById('s" + to_string(client_index) + "').innerHTML+=\"" + tempstr + "\";</script>\n";

									int i = 0;
									for (i = 0; i < sendstr.length(); i++) {
										tempdata[i] = sendstr[i];
									}


									_socket->async_send(
										boost::asio::buffer(tempdata, i),
										[this](boost::system::error_code ec, std::size_t length/* length */) {
										if (!ec) {
											printf("after send to browser\n");
										}
									}
									);
									//std::cout << tempstr << std::endl;
									tempstr.clear();
									sendstr.clear();
								}
							}
							else if (_data[index] != '\x00') {
								if (_data[index] == '<') {
									//要用html encode才可以在網頁上顯示html tag
									tempstr += "&lt;";
								}
								else if (_data[index] == '>') {
									tempstr += "&gt;";
								}
								else if (_data[index] == '\r' || _data[index] == '\x08') {
									//這一步卡了我好久= = 
								}
								else if (_data[index] == '\"') {
									tempstr += "&#x22;";
								}
								else if (_data[index] == '\'') {
									tempstr += "&#x27;";
								}
								else {
									tempstr += _data[index];
								}
							}
							//printf("%d bytes: %c : %hhd\n",index,_data[index],_data[index]);
							if (_data[index] == '\x00') {
								//std::cout << "<script>document.getElementById('s" << client_index << "').innerHTML+=\"" << tempstr << "\";</script>" << std::endl;
								sendstr = "<script>document.getElementById('s" + to_string(client_index) + "').innerHTML+=\"" + tempstr + "\";</script>\n";
								int i = 0;
								for (i = 0; i < sendstr.length(); i++) {
									tempdata[i] = sendstr[i];
								}


								_socket->async_send(
									boost::asio::buffer(tempdata, i),
									[this](boost::system::error_code ec, std::size_t length/* length */) {
									if (!ec) {
										printf("after send to browser\n");
									}
								}
								);


								sendstr.clear();
								tempstr.clear();
								break;
							}
							index++;
						}
					}


					if (active_flag == 1)
						do_write();
					do_read();

				}
				else {
					//std::cout<<"read ec :"<<ec<<std::endl;
				}
			}
			);


		}

		void do_write() {


			auto self(shared_from_this());

			//std::cout<<"getline"<<std::endl;
			char request[max_length];
			std::string line;
			std::string tempstr;
			size_t len = 0;


			if (std::getline(file, tempstr)) {
				int index = 0;
				for (index = 0; index<tempstr.length(); index++) {
					if (tempstr[index] == '<') {
						//要用html encode才可以在網頁上顯示html tag
						line += "&lt;";
					}
					else if (tempstr[index] == '>') {
						line += "&gt;";
					}
					else if (tempstr[index] == '\r' || tempstr[index] == '\x08' || tempstr[index] == '\n') {
						//這一步卡了我好久= = 
					}
					else if (tempstr[index] == '\"') {
						line += "&#x22;";
					}
					else if (tempstr[index] == '\'') {
						line += "&#x27;";
					}
					else {
						line += tempstr[index];
					}
				}
				array<char, 3000> tempdata;
				array<char, 3000> sendtempdata;

				std::string sendstr;

				//std::cout << "<script>document.getElementById('s" << client_index << "').innerHTML += '<b>" << line << "&NewLine;</b>';</script>" << std::endl;
				sendstr = "<script>document.getElementById('s" + to_string(client_index) + "').innerHTML += '<b>" + line + "&NewLine;</b>';</script>\n";

				int i = 0;
				for (i = 0; i < sendstr.length(); i++) {
					tempdata[i] = sendstr[i];
				}

				_socket->async_send(
					boost::asio::buffer(tempdata, i),
					[this](boost::system::error_code ec, std::size_t length/* length */) {
					if (!ec) {
						printf("after send to browser\n");
					}
				}
				);


				tempstr = tempstr + "\n";

				for (i = 0; i < tempstr.length(); i++) {
					sendtempdata[i] = tempstr[i];
				}


				s.async_send(
					boost::asio::buffer(sendtempdata, i),
					[this, self, request](boost::system::error_code ec, std::size_t length/* length */) {
					if (!ec) {
					}
				}
				);
			}
			else {
				//do nothing?
			}

		}

	};
/************************************/



public:
	std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
	EchoSession(boost::asio::ip::tcp::socket socket) : _socket(std::make_shared<boost::asio::ip::tcp::socket>(move(socket))) {}
	

	void start() {
		//do_read(); 
		do_read_with_head_from_client();
	}

private:
	void do_read_with_head_from_client() {
		//讀取來自客戶端的訊息(標頭)
		auto self(shared_from_this());
		_socket->async_read_some(
			buffer(_data, max_length),
			[this, self](boost::system::error_code ec, std::size_t length) {
			for (int i = 0; i < length; i++) {
				printf("%c", _data[i]);
			}
			printf("\n");

			parse_header();

			send_OK();

			if (parameters[15][0] == 'c') {
				do_console();
			}
			else if (parameters[15][0] == 'p') {
				do_panel();
			}
		}
		);

	}

	void send_OK() {

		auto self(shared_from_this());

		array<char, 20> okhead = { "HTTP/1.1 200 OK\r\n\0" };

		int index = 0;
		while (1) {
			_data[index] = okhead[index];
			if (okhead[index] != '\0') {
				index++;
			}
			else {
				break;
			}

		}
		_socket->async_write_some(
			buffer(_data, index),
			[this, self](boost::system::error_code ec, std::size_t length) {
		});

	}

	void parse_header() {
		//initialize parameters
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 50; j++) {
				parameters[i][j] = '\x00';
			}
		}

		int index = 0;
		int state = 0;
		int tindex = 0;

		//_data to char array
		index = 0;
		while (1) {
			temp_header[index] = _data[index];
			printf("%c", temp_header[index]);

			if (_data[index] == '\n') {
				temp_header[index] = '\x00';
				break;
			}
			index++;
		}

		index = 0;

		while (1) {
			printf("%c", temp_header[index]);
			if (temp_header[index] == '/') {
				state = 1;
				index++;
			}
			else if (state == 1 && (temp_header[index] == '?' || temp_header[index] == ' ')) {
				printf("break!\n");
				break;
			}
			if (state == 1) {
				parameters[15][tindex++] = temp_header[index];
			}
			index++;
		}
		parameters[15][tindex] = '\x00';

		if (temp_header[index - 1] == ' ') {
			//沒有任何參數，跳離function
			return;
		}

		state = 0;
		tindex = 0;
		for (int i = 0; i < 15; i++) {
			printf("hi head \n");
			state = 0;
			tindex = 0;
			while (1) {
				if (temp_header[index] == '=') {
					state = 1;
					index++;
				}
				if (temp_header[index] == '&' || temp_header[index] == ' ') {
					index++;
					break;
				}
				if (state == 1) {
					parameters[i][tindex++] = temp_header[index];
				}
				index++;
			}
			if (temp_header[index - 1] == ' ') {
				//no more parameters
				break;
			}
			parameters[i][tindex] = '\x00';
		}

		for (int i = 0; i < 16; i++) {
			printf("parameters[%d] %s\n", i, parameters[i]);
		}
		printf("%s\n", parameters[15]);
	}

	void do_panel() {
		auto self(shared_from_this());

		std::cout << "do_panellll!\n" << std::endl;

		std::string sendstr;

		//sendstr += "a";
		//sendstr += '\x00';
		//printf("%d", sendstr.length());

		

		sendstr += "Content-type: text/html\n\n";

		sendstr += "<!DOCTYPE html>\n";
		sendstr += "  <html lang=\"en\">\n";
		sendstr += "  <head>\n";
		sendstr += "    <title>NP Project 3 Panel</title>\n";
		sendstr += "    <link\n";
		sendstr += "      rel=\"stylesheet\"\n";
		sendstr += "      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n";
		sendstr += "      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n";
		sendstr += "      crossorigin=\"anonymous\"\n";
		sendstr += "    />\n";
		sendstr += "    <link\n";
		sendstr += "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n";
		sendstr += "      rel=\"stylesheet\"\n";
		sendstr += "    />\n";
		sendstr += "    <link\n";
		sendstr += "      rel=\"icon\"\n";
		sendstr += "      type=\"image / png\"\n";
		sendstr += "      href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\n";
		sendstr += "    />\n";
		sendstr += "    <style>\n";
		sendstr += "      * {\n";
		sendstr += "        font-family: 'Source Code Pro', monospace;\n";
		sendstr += "      }\n";
		sendstr += "    </style>\n";
		sendstr += "  </head>\n";
		sendstr += "  <body class=\"bg-secondary pt-5\">\n";
		sendstr += "    <form action=\"console.cgi\" method=\"GET\">\n";
		sendstr += "      <table class=\"table mx-auto bg-light\" style=\"width: inherit\">\n";
		sendstr += "        <thead class=\"thead-dark\">\n";
		sendstr += "          <tr>\n";
		sendstr += "            <th scope=\"col\">#</th>\n";
		sendstr += "            <th scope=\"col\">Host</th>\n";
		sendstr += "            <th scope=\"col\">Port</th>\n";
		sendstr += "            <th scope=\"col\">Input File</th>\n";
		sendstr += "          </tr>\n";
		sendstr += "        </thead>\n";
		sendstr += "        <tbody>\n";
		sendstr += "          <tr>\n";
		sendstr += "            <th scope=\"row\" class=\"align-middle\">Session 1</th>\n";
		sendstr += "            <td>\n";
		sendstr += "              <div class=\"input-group\">\n";
		sendstr += "                <select name=\"h0\" class=\"custom-select\">\n";
		sendstr += "                  <option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\n";
		sendstr += "                </select>\n";
		sendstr += "                <div class=\"input-group-append\">\n";
		sendstr += "                  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n";
		sendstr += "                </div>\n";
		sendstr += "              </div>\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <input name=\"p0\" type=\"text\" class=\"form-control\" size=\"5\" />\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <select name=\"f0\" class=\"custom-select\">\n";
		sendstr += "                <option></option>\n";
		sendstr += "                <option>t1.txt</option>\n";
		sendstr += "                <option>t2.txt</option>\n";
		sendstr += "                <option>t3.txt</option>\n";
		sendstr += "                <option>t4.txt</option>\n";
		sendstr += "                <option>t5.txt</option>\n";
		sendstr += "                <option>t6.txt</option>\n";
		sendstr += "                <option>t7.txt</option>\n";
		sendstr += "                <option>t8.txt</option>\n";
		sendstr += "                <option>t9.txt</option>\n";
		sendstr += "                <option>t10.txt</option>\n";
		sendstr += "              </select>\n";
		sendstr += "            </td>\n";
		sendstr += "          </tr>\n";
		sendstr += "          <tr>\n";
		sendstr += "            <th scope=\"row\" class=\"align-middle\">Session 2</th>\n";
		sendstr += "            <td>\n";
		sendstr += "              <div class=\"input-group\">\n";
		sendstr += "                <select name=\"h1\" class=\"custom-select\">\n";
		sendstr += "                  <option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\n";
		sendstr += "                </select>\n";
		sendstr += "                <div class=\"input-group-append\">\n";
		sendstr += "                  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n";
		sendstr += "                </div>\n";
		sendstr += "              </div>\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <input name=\"p1\" type=\"text\" class=\"form-control\" size=\"5\" />\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <select name=\"f1\" class=\"custom-select\">\n";
		sendstr += "                <option></option>\n";
		sendstr += "                <option>t1.txt</option>\n";
		sendstr += "                <option>t2.txt</option>\n";
		sendstr += "                <option>t3.txt</option>\n";
		sendstr += "                <option>t4.txt</option>\n";
		sendstr += "                <option>t5.txt</option>\n";
		sendstr += "                <option>t6.txt</option>\n";
		sendstr += "                <option>t7.txt</option>\n";
		sendstr += "                <option>t8.txt</option>\n";
		sendstr += "                <option>t9.txt</option>\n";
		sendstr += "                <option>t10.txt</option>\n";
		sendstr += "              </select>\n";
		sendstr += "            </td>\n";
		sendstr += "          </tr>\n";
		sendstr += "          <tr>\n";
		sendstr += "            <th scope=\"row\" class=\"align-middle\">Session 3</th>\n";
		sendstr += "            <td>\n";
		sendstr += "              <div class=\"input-group\">\n";
		sendstr += "                <select name=\"h2\" class=\"custom-select\">\n";
		sendstr += "                  <option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\n";
		sendstr += "                </select>\n";
		sendstr += "                <div class=\"input-group-append\">\n";
		sendstr += "                  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n";
		sendstr += "                </div>\n";
		sendstr += "              </div>\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <input name=\"p2\" type=\"text\" class=\"form-control\" size=\"5\" />\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <select name=\"f2\" class=\"custom-select\">\n";
		sendstr += "                <option></option>\n";
		sendstr += "                <option>t1.txt</option>\n";
		sendstr += "                <option>t2.txt</option>\n";
		sendstr += "                <option>t3.txt</option>\n";
		sendstr += "                <option>t4.txt</option>\n";
		sendstr += "                <option>t5.txt</option>\n";
		sendstr += "                <option>t6.txt</option>\n";
		sendstr += "                <option>t7.txt</option>\n";
		sendstr += "                <option>t8.txt</option>\n";
		sendstr += "                <option>t9.txt</option>\n";
		sendstr += "                <option>t10.txt</option>\n";
		sendstr += "              </select>\n";
		sendstr += "            </td>\n";
		sendstr += "          </tr>\n";
		sendstr += "          <tr>\n";
		sendstr += "            <th scope=\"row\" class=\"align-middle\">Session 4</th>\n";
		sendstr += "            <td>\n";
		sendstr += "              <div class=\"input-group\">\n";
		sendstr += "                <select name=\"h3\" class=\"custom-select\">\n";
		sendstr += "                  <option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\n";
		sendstr += "                </select>\n";
		sendstr += "                <div class=\"input-group-append\">\n";
		sendstr += "                  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n";
		sendstr += "                </div>\n";
		sendstr += "              </div>\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <input name=\"p3\" type=\"text\" class=\"form-control\" size=\"5\" />\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <select name=\"f3\" class=\"custom-select\">\n";
		sendstr += "                <option></option>\n";
		sendstr += "                <option>t1.txt</option>\n";
		sendstr += "                <option>t2.txt</option>\n";
		sendstr += "                <option>t3.txt</option>\n";
		sendstr += "                <option>t4.txt</option>\n";
		sendstr += "                <option>t5.txt</option>\n";
		sendstr += "                <option>t6.txt</option>\n";
		sendstr += "                <option>t7.txt</option>\n";
		sendstr += "                <option>t8.txt</option>\n";
		sendstr += "                <option>t9.txt</option>\n";
		sendstr += "                <option>t10.txt</option>\n";
		sendstr += "              </select>\n";
		sendstr += "            </td>\n";
		sendstr += "          </tr>\n";
		sendstr += "          <tr>\n";
		sendstr += "            <th scope=\"row\" class=\"align-middle\">Session 5</th>\n";
		sendstr += "            <td>\n";
		sendstr += "              <div class=\"input-group\">\n";
		sendstr += "                <select name=\"h4\" class=\"custom-select\">\n";
		sendstr += "                  <option></option><option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option><option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option><option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option><option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option><option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option><option value=\"npbsd1.cs.nctu.edu.tw\">npbsd1</option><option value=\"npbsd2.cs.nctu.edu.tw\">npbsd2</option><option value=\"npbsd3.cs.nctu.edu.tw\">npbsd3</option><option value=\"npbsd4.cs.nctu.edu.tw\">npbsd4</option><option value=\"npbsd5.cs.nctu.edu.tw\">npbsd5</option>\n";
		sendstr += "                </select>\n";
		sendstr += "                <div class=\"input-group-append\">\n";
		sendstr += "                  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n";
		sendstr += "                </div>\n";
		sendstr += "              </div>\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <input name=\"p4\" type=\"text\" class=\"form-control\" size=\"5\" />\n";
		sendstr += "            </td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <select name=\"f4\" class=\"custom-select\">\n";
		sendstr += "                <option></option>\n";
		sendstr += "                <option>t1.txt</option>\n";
		sendstr += "                <option>t2.txt</option>\n";
		sendstr += "                <option>t3.txt</option>\n";
		sendstr += "                <option>t4.txt</option>\n";
		sendstr += "                <option>t5.txt</option>\n";
		sendstr += "                <option>t6.txt</option>\n";
		sendstr += "                <option>t7.txt</option>\n";
		sendstr += "                <option>t8.txt</option>\n";
		sendstr += "                <option>t9.txt</option>\n";
		sendstr += "                <option>t10.txt</option>\n";
		sendstr += "              </select>\n";
		sendstr += "            </td>\n";
		sendstr += "          </tr>\n";
		sendstr += "          <tr>\n";
		sendstr += "            <td colspan=\"3\"></td>\n";
		sendstr += "            <td>\n";
		sendstr += "              <button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\n";
		sendstr += "            </td>\n";
		sendstr += "          </tr>\n";
		sendstr += "        </tbody>\n";
		sendstr += "      </table>\n";
		sendstr += "    </form>\n";
		sendstr += "  </body>\n";
		sendstr += "</html>\n";

		sendstr += '\x00';
		std::cout << sendstr << std::endl;

		for (int i = 0; i < sendstr.length(); i++) {
			_data[i] = sendstr[i];
		}

		_socket->async_write_some(
			buffer(_data, sendstr.length()),
			[this, self](boost::system::error_code ec, std::size_t length) {
		});
	}

	void do_console() {
		
		printf("do_console!\n");

		auto self(shared_from_this());

		std::string sendstr;

		int number_of_console=0;

		for (int i = 0; i < 15; i++) {
			if ((i % 3) == 0) {
				if (parameters[i][0] != '\x00') {
					number_of_console++;
				}
			}
		}

		//new clientSession object
		//std::make_shared<clientSession>(1)->start();
		
		sendstr += "Content-type: text/html\n\n";

		sendstr += "<!DOCTYPE html>\n";
		sendstr += "<html lang=\"en\">\n";
		sendstr += "  <head>\n";
		sendstr += "    <meta charset=\"UTF-8\" />\n";
		sendstr =sendstr+ "    <title>NP Project "+to_string(number_of_console)+" Console</title>\n";
		sendstr += "    <link\n";
		sendstr += "      rel=\"stylesheet\"\n";
		sendstr += "      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n";
		sendstr += "      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n";
		sendstr += "      crossorigin=\"anonymous\"\n";
		sendstr += "    />\n";
		sendstr += "    <link\n";
		sendstr += "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n";
		sendstr += "      rel=\"stylesheet\" << std::endl;\n";
		sendstr += "    />\n";
		sendstr += "    <link\n";
		sendstr += "      rel=\"icon\"\n";
		sendstr += "      type=\"image/png\"\n";
		sendstr += "      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n";
		sendstr += "    />\n";
		sendstr += "    <style>\n";
		sendstr += "      * {\n";
		sendstr += "        font-family: 'Source Code Pro' , monospace;\n";
		sendstr += "        font-size: 1rem !important;\n";
		sendstr += "      }\n";
		sendstr += "      body  {\n";
		sendstr += "            background-color: #212529;\n";
		sendstr += "      }\n";
		sendstr += "      pre  {\n";
		sendstr += "        color: #cccccc;\n";
		sendstr += "      }\n";
		sendstr += "      b  {\n";
		sendstr += "        color: #ffffff;\n";
		sendstr += "      }\n";
		sendstr += "    </style>\n";
		sendstr += "  </head>\n";
		sendstr += "  <body>\n";
		sendstr += "    <table class=\"table table-dark table-bordered\">\n";
		sendstr += "      <thead>\n";
		sendstr += "        <tr>\n";

		for(int i=0;i<15;i+=3){
			if(parameters[i][0]!='\x00'){
				std::string str1(parameters[i]);
				std::string str2(parameters[i + 1]);

				sendstr =sendstr+ "          <th scope=\"col\">"+str1+":"+str2+"</th>\n";
			}
		}

		sendstr += "        </tr>\n";
		sendstr += "      </thead>\n";
		sendstr += "      <tbody>\n";
		sendstr += "        <tr>\n";

		for(int i=0 ; i<15 ;i+=3){
			if(parameters[i][0]!='\x00'){
				sendstr =sendstr+"          <td><pre id =\"s"+to_string(i/3)+"\" class=\"mb-0\"></pre></td>\n";
			}
		}

		sendstr += "        </tr>\n";
		sendstr += "      </tbody>\n";
		sendstr += "    </table>\n";
		sendstr += "  </body>\n";
		sendstr += "</html>\n";
		sendstr += '\x00';
		
		std::cout << sendstr << std::endl;

		for (int i = 0; i < sendstr.length(); i++) {
			_data[i] = sendstr[i];
		}

		_socket->async_write_some(
			buffer(_data, sendstr.length()),
			[this, self](boost::system::error_code ec, std::size_t length) {
				for (int i = 0; i < 15; i += 3) {
					if (parameters[i][0] != '\x00') {
						//clientSession(char *name,char *port, char *filename,int client_index,char parameters[16][50],boost::asio::ip::tcp::socket brower_s) : 
						//std::make_
						//shared<clientSession>(parameters[i], parameters[i + 1], parameters[i + 2], i / 3,parameters,move(_socket))->start();
						std::make_shared<clientSession>(parameters[i], parameters[i + 1], parameters[i + 2], i / 3, _socket/*,move(_socket)*/)->start();
						//printf("%s %s %s \n", parameters[i], parameters[i + 1], parameters[i + 2]);
					}
				}
		});

		
		
	}

	

	void do_read() {
		auto self(shared_from_this());
		_socket->async_read_some(
			buffer(_data, max_length),
			[this, self](boost::system::error_code ec, size_t length) {
			if (!ec) do_write(length);
		});
	}

	void do_write(size_t length) {
		auto self(shared_from_this());
		_socket->async_send(
			buffer(_data, length),
			[this, self](boost::system::error_code ec, size_t /* length */) {
			if (!ec) do_read();
		});
	}
};

class EchoServer {
private:
	ip::tcp::acceptor _acceptor;
	ip::tcp::socket _socket;

public:
	EchoServer(short port)
		: _acceptor(global_io_service, ip::tcp::endpoint(ip::tcp::v4(), port)),
		_socket(global_io_service) {
		do_accept();
	}

private:
	void do_accept() {
		_acceptor.async_accept(_socket, [this](boost::system::error_code ec) {
			if (!ec) make_shared<EchoSession>(move(_socket))->start();

			do_accept();
		});
	}
};

int main(int argc, char* const argv[]) {
	if (argc != 2) {
		cerr << "Usage:" << argv[0] << " [port]" << endl;
		return 1;
	}

	try {
		unsigned short port = atoi(argv[1]);
		EchoServer server(port);
		global_io_service.run();
	}
	catch (exception& e) {
		cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
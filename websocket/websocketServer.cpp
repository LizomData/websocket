#define _CRT_SECURE_NO_WARNINGS

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <codecvt>
#include <wchar.h>
#include <locale.h>
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
std::wstring string_to_wstring(const std::string& str)
{
	std::wstring r;
	const char* source = str.c_str();
	wchar_t* dest = NULL;
	int len = 0;
	int ret = 0;
	len = strlen(source) + 1;
	if (len <= 1)
		return 0;
	dest = new wchar_t[len];
	ret = mbstowcs(dest, source, len);
	r = std::wstring(dest);
	delete[] dest;
	return r;
}

std::string wstring_to_string(const std::wstring& ws)
{
	std::string r = "";
	const wchar_t* source = ws.c_str();
	char* dest = NULL;
	int len = 0;
	int ret = 0;
	len = wcslen(source) + 1;
	if (len <= 1)
		return 0;
	dest = new char[len * sizeof(wchar_t)];
	ret = wcstombs(dest, source, len * sizeof(wchar_t));
	r = std::string(dest);
	delete[] dest;
	return r;
}

std::string ansi_to_utf8(const std::string& s)
{
	static std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return conv.to_bytes(string_to_wstring(s));
}
std::string utf8_to_ansi(const std::string& s)
{
	static std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
	return wstring_to_string(conv.from_bytes(s));
}

void do_session(tcp::socket& socket)
{
	try
	{
		websocket::stream<tcp::socket> ws{ std::move(socket) };

		ws.set_option(websocket::stream_base::decorator(
			[](websocket::response_type& res)
			{
				res.set(http::field::server,
					std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
				res.set(http::field::access_control_allow_origin, "*");
				res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
				res.set(http::field::access_control_allow_headers, "Content-Type");
			}));

		ws.accept();//等待客户端连接
		for (;;)
		{
			std::string text = "80.2,50.3,8.1|0.2,0.3,8.1";
			ws.write(net::buffer(ansi_to_utf8(text)));// 发送消息


			//beast::flat_buffer buffer;// 这个缓冲区将保存传入的消息
			//ws.read(buffer);// 读取一条消息
			//auto out = beast::buffers_to_string(buffer.cdata());
			//std::cout << utf8_to_ansi(out) << std::endl;

			Sleep(1000);
			/*
			// 将读取到的消息再发送回客户端
			ws.text(ws.got_text());
			ws.write(buffer.data());
			*/
		}
	}
	catch (beast::system_error const& se)
	{
		if (se.code() != websocket::error::closed)
			std::cerr << "Error: " << se.code().message() << std::endl;
	}
	catch (std::exception const& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

int main(int argc, char* argv[])
{
	try
	{
		auto const address = net::ip::make_address("0.0.0.0");//绑定ip地址
		auto const port = static_cast<unsigned short>(std::atoi("1234"));//绑定端口号
		net::io_context ioc{ 1 };
		tcp::acceptor acceptor{ ioc,{ address, port } };
		for (;;)
		{
			tcp::socket socket{ ioc };
			acceptor.accept(socket);
			// 开启线程等待客户端的连接请求
			std::thread{ std::bind(&do_session,std::move(socket)) }.detach();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}

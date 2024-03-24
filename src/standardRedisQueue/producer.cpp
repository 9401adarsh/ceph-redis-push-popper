#include <boost/redis/src.hpp>
#include <boost/redis/connection.hpp>
#include <boost/redis/request.hpp>
#include <boost/redis/response.hpp>
#include <boost/redis/config.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>

namespace asio = boost::asio;
using boost::redis::connection;
using boost::redis::request;
using boost::redis::response;
using boost::redis::config;

auto main(int argc, char * argv[])-> int 
{
    try
    {
        config cfg;

        if(argc < 3)
        {
            std::string errorMsg = "Incorrect Usage. Usage: ./producer.cpp [Name of Queue] [list of elements to push]";
            throw std::invalid_argument(errorMsg);
        } 
 
        cfg.addr.host = "127.0.0.1";
        cfg.addr.port = "6379";

        std::string queueName = argv[1];

        std::list<std::string> elements; 

        for(int i = 2; i < argc; i++)
        {
            std::string element = std::string(argv[i]);
            elements.push_back(element);
        }

        request req; 
        response<int> resp;

        req.push_range("RPUSH", queueName,  elements);

        asio::io_context ioc; 
        connection conn{ioc};

        conn.async_run(cfg, {}, asio::detached);

        conn.async_exec(req, resp, [&](auto ec, auto) {
         if (!ec)
            std::cout << "Length of Queue after push: " << std::get<0>(resp).value() << std::endl;
         conn.cancel();
        });

        ioc.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

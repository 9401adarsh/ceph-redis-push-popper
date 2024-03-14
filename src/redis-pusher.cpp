#include <boost/redis/src.hpp>
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

        if(argc < 5)
        {
            std::string errorMsg = "Incorrect Usage. Usage: ./redis-pusher.cpp [HOST] [PORT] [Name of Queue] [list of elements to push]";
            throw std::invalid_argument(errorMsg);
        } 
 
        cfg.addr.host = argv[1];
        cfg.addr.port = argv[2];

        std::string queueName = argv[3];

        std::list<std::string> elements; 

        for(int i = 4; i < argc; i++)
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

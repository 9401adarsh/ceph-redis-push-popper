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

        if(argc < 4)
        {
            std::string errorMsg = "Incorrect Usage. Usage: ./redis-pusher.cpp [HOST] [PORT] [Name of Queue] [optional: Number of Elements to Pop]";
            throw std::invalid_argument(errorMsg);
        } 
 
        cfg.addr.host = argv[1];
        cfg.addr.port = argv[2];

        std::string queueName = argv[3];
        int numToPop = argc == 5 ? std::stoi(argv[4]): 1;

        request req; 
        response<std::vector<std::string>> resp;

        if(argc == 5)
            req.push("LPOP", queueName, numToPop);
        else
            req.push("LPOP", queueName);

        asio::io_context ioc; 
        connection conn{ioc};

        conn.async_run(cfg, {}, asio::detached);

        conn.async_exec(req, resp, [&](auto ec, auto) {
         if (!ec)
            std::cout << "Popped the following element:  "<< std::get<0>(resp).value()[0] << std::endl;
         conn.cancel();
        });

        ioc.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

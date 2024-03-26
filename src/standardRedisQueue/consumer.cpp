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

        if(argc < 2)
        {
            std::string errorMsg = "Incorrect Usage. Usage: ./consumer.cpp [Name of Queue] [optional: Number of Elements to Pop]";
            throw std::invalid_argument(errorMsg);
        } 
 
        cfg.addr.host = "127.0.0.1";
        cfg.addr.port = "6379";

        std::string queueName = argv[1];
        int numToPop = argc == 3 ? std::stoi(argv[2]): 1;

        request req; 
        response<std::vector<std::string>> resp;

        if(argc == 3)
            req.push("LPOP", queueName, numToPop);
        else
            req.push("LPOP", queueName);

        asio::io_context ioc; 
        connection conn{ioc};

        conn.async_run(cfg, {}, asio::detached);

        conn.async_exec(req, resp, [&](auto ec, auto) {
         if (!ec){ 
            std::vector<std::string> poppedElements = std::get<0>(resp).value();

            if(poppedElements.size() > 0)
            {    
                std::cout << "Popped the following elements: ";
                for(auto &x: std::get<0>(resp).value())
                    std::cout<<x<<"\t";
                std::cout<<std::endl;
            }
            else 
                std::cout << "Queue is empty, nothing to pop."<< std::endl;   
         }
            
         conn.cancel();
        });

        ioc.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}

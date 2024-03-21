// This Program is a simple implementation of the Consumer in 2 Phase Commit Queue Protocol using Redis Lists.

/*
Have two redis queues:
    i) topic:reserve
    ii) topic:commit

To Do:

i) set up redis config
ii) check if commit queue, has elements
        if it has elements, run a blocking RPOP from the element
        take the element
        send notification to client, wait for ack
        if ack:
            remove the element from the commit queue, as notif has been sent
        else
            let it persist in the queue until we a ack

iii) periodic checker for reservations and commit queue, can only persist for a while



iv) Locks for the resources, need to think on how to proceed.

*/

#include <boost/redis/src.hpp>
#include <boost/redis/connection.hpp>
#include <boost/redis/request.hpp>
#include <boost/redis/response.hpp>
#include <boost/redis/config.hpp>
#include <boost/asio/detached.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>

namespace asio = boost::asio;
namespace redis = boost::redis;

redis::config redisConfigSetup(std::string host, std::string port)
{
    redis::config cfg;

    cfg.addr.host = host;
    cfg.addr.port = port;

    return cfg;
}

enum notifDeliveryStatus
{
    ACK,
    NACK
};

notifDeliveryStatus performNotificationDelivery(std::string notif)
{
    std::cout << "Performing Notification Delivery... Notif: " + notif << std::endl;
    notifDeliveryStatus status;
    sleep(10);

    // Assuming the notification delivery system fails 5% of the time.
    srand(time(nullptr));
    if (rand() % 3 == 0)
        status = NACK;
    else
        status = ACK;

    return status;
}

void accquireLock()
{
}

void releaseLock()
{
}

void deleteNotification(asio::io_context &eventThread, redis::config &cfg, std::string redisResource, std::string notif)
{

    redis::connection conn{eventThread};

    redis::request req;
    redis::response<int> resp;

    req.clear();
    req.push("LREM", redisResource, -1, notif);

    conn.async_run(cfg, {}, asio::detached);
    conn.async_exec(req, resp, [&](auto errorCode, auto)
                    {
        
        if(!errorCode)
        {
            if(std::get<0>(resp).value() == 1)
                std::cout<<"Deleted the notif " + notif + " from commit queue..."<<std::endl;
            else
                std::cout<<"Error During Delete...."<<std::endl;
        }

        conn.cancel(); });

    eventThread.run();
    return;
}

void fetchNotification(asio::io_context &eventThread, redis::config &cfg, std::string redisResource)
{

    redis::connection conn{eventThread};

    redis::request req;
    redis::response<std::vector<std::string>> resp;

    req.clear();
    req.push("LRANGE", redisResource, -5, -1);

    conn.async_run(cfg, {}, asio::detached);
    conn.async_exec(req, resp, [&](auto errorCode, auto)
                    {
        
        if(!errorCode)
        {
            std::vector<std::string> notifVec = std::get<0>(resp).value(); 
            if(notifVec.size() < 5)
            {
 
                std::cout<<"Not enough notifications on the commit Queue, waiting for atleast 5 notifications to commit...."<<std::endl;
                asio::io_context childEventThread;
                fetchNotification(childEventThread, cfg, redisResource);

            }
            else {

                for(auto &notif: notifVec)
                {   
                    notifDeliveryStatus status = performNotificationDelivery(notif);
                if(status == ACK) {
                    
                    std::cout<<"Notification ACKed. Deleting notif from the queue"<<std::endl;
                    asio::io_context childEventThread; 
                    deleteNotification(childEventThread, cfg, redisResource, notif);

                }
                else {
                    std::cout<<"Notification NACKed. Will have to try again later..."<<std::endl;
                }

                }
                
            }

        }

        conn.cancel(); });

    eventThread.run();
    return;
}

void consumerThread(redis::config &cfg, std::string redisResource)
{
    asio::io_context eventThread;
    fetchNotification(eventThread, cfg, redisResource);
}

int main()
{

    auto cfg = redisConfigSetup("127.0.0.1", "6379");

    int i = 1;
    while (true)
    {
        std::cout << "Run " << i++ << std::endl;
        consumerThread(cfg, "topic:commit");
        sleep(3);
        std::cout << "---------------------" << std::endl;
    }
}

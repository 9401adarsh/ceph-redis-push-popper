/*
i) Set Up Configuration Function using boost::redis::config, set host and port etc.
iii) Wait until we can Acquire Lock for topic:reserve. Reserve a spot on Redis Queue. Release Lock
iv) Perform RADOS operation.
    a) If Rados Operation succesful
        wait until both locks for topic:reserve and topic:commit accquired
        commit the reservation
        release both locks
    b) Else
        wait until locks for topic:reserve accquired
        abort the reservation
        release lock


Have two redis queues:
    i) topic:reserve
    ii) topic:commit

To Do:


i) Periodically Remove Reservations on the Reserve Queue.
ii) Locks for Redis Resources.
ii) Refactor - Random Strings to mimic as Request Strings ideally - not important
iii) Account for commit operation failures in those cases, log the commmit operation error


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
#include <random>

namespace asio = boost::asio;
namespace redis = boost::redis;

std::string getRandomString()
{

    const std::vector<std::string> randomStrings = {
        "jV3a9kW6pR", "5FbT1sGcYz", "eD7hN6qA2x", "rL4jP9mX8o", "yU2wK3vS1b",
        "tI5nH4fC7m", "oE8gM2dZ6q", "pO1rV7lY3x", "aT9eI6kF8l", "uS2vR5jG1q",
        "iF4cO3hN9x", "dW7mX8yZ6k", "bQ2aT1rP5s", "cE9nU6wK3v", "zL4tI7oH2f",
        "fD5gM8pR9y", "mX1bS6uV3l", "wK9aP4jG7n", "qC3eT5vR2i", "vH6lY8zN1o"};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, randomStrings.size() - 1);

    int randomIndex = dis(gen);

    return randomStrings[randomIndex];
}

redis::config redisConfigSetup(std::string host, std::string port)
{
    redis::config cfg;

    cfg.addr.host = host;
    cfg.addr.port = port;

    return cfg;
}

// wrapper for performing RADOS Operation

enum radosOperationStatus
{
    RGW_OP_SUCCESS,
    RGW_OP_FAILURE
};

std::pair<std::string, radosOperationStatus> doRadosOperation(std::string radosRequestString)
{
    radosOperationStatus status;

    std::cout << "Performing Rados Operation for Request: " << radosRequestString << std::endl;

    // random hardcoded payload string, that mimics as output of the rados operation.
    std::string payload = radosRequestString;

    sleep(10);
    // Assuming Rados Operation fails 5% of the time.

    srand(time(nullptr));
    if (rand() % 20 == 0)
        status = RGW_OP_FAILURE;
    else
        status = RGW_OP_SUCCESS;

    return {payload, status};
}

// To Do - Add Locks

void accquireLock(std::string redisResource)
{
}

void releaseLock(std::string redisResource)
{
}

void reserveOperation(asio::io_context &eventThread, redis::config &cfg, std::string redisResource, std::string radosRequestString)
{

    redis::connection conn{eventThread};

    redis::request req;
    redis::response<int> resp;

    req.clear();
    req.push("LPUSH", redisResource, radosRequestString);

    conn.async_run(cfg, {}, asio::detached);
    conn.async_exec(req, resp, [&](auto errorCode, auto)
                    {
    if (!errorCode)
        std::cout << "Space Reserved on Reserve Queue. Last Request String: " + radosRequestString<<std::endl;
        conn.cancel(); });

    eventThread.run();

    return;
}

void commitOrAbortOperation(asio::io_context &eventThread, redis::config &cfg, std::pair<std::string, std::string> redisResources, std::pair<std::string, radosOperationStatus> radosOperationOutput)
{

    redis::connection conn{eventThread};

    redis::request req;

    std::string tmpQueue = redisResources.first;
    std::string commitQueue = redisResources.second;

    std::string payload = radosOperationOutput.first;
    radosOperationStatus status = radosOperationOutput.second;

    if (status == RGW_OP_SUCCESS)
    {

        std::cout << "Rados Operation Successful..." << std::endl;
        redis::response<std::string, redis::ignore_t> resp;

        req.push("RPOPLPUSH", tmpQueue, commitQueue);
        req.push("LREM", tmpQueue, 1, payload);

        conn.async_run(cfg, {}, asio::detached);
        conn.async_exec(req, resp, [&](auto errorCode, auto)
                        {
        if (!errorCode)
            std::cout << "Commiting Notif:" << std::get<0>(resp).value() << std::endl;
        conn.cancel(); });

        eventThread.run();
    }
    else
    {

        std::cout << "Rados Operation Failed!!!" << std::endl;
        redis::response<redis::ignore_t> resp;
        req.push("LREM", tmpQueue, 1, payload);

        conn.async_run(cfg, {}, asio::detached);
        conn.async_exec(req, resp, [&](auto errorCode, auto)
                        {
        if (!errorCode)
            std::cout << "Aborting the Reservation for " + payload<< std::endl;
            conn.cancel(); });

        eventThread.run();
    }
}

void producerThread(redis::config &cfg)
{

    std::string radosRequest = getRandomString();

    asio::io_context reserveOpCtxt;
    reserveOperation(reserveOpCtxt, cfg, "topic:reserve", radosRequest);

    auto radosOutput = doRadosOperation(radosRequest);

    asio::io_context commitOrAbortOpCtxt;
    commitOrAbortOperation(commitOrAbortOpCtxt, cfg, {"topic:reserve", "topic:commit"}, radosOutput);
}

int main()
{
    auto cfg = redisConfigSetup("127.0.0.1", "6379");
    while (true)
    {
        producerThread(cfg);

        std::cout << "-----------------" << std::endl;
        sleep(3);
    }

    return 0;
}

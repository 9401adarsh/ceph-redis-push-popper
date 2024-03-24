## Evaluation Task 2 for Rados to Redis - Ceph GSOC 24

### To Do:
- [x] Implement a C++ client that pushes to a Redis Queue using Boost's Redis library.
- [x] Implement a C++ client that pops from a Redis Queue using Boost's Redis library.
- [ ] (optional) Write tests for the above two clients using GTest.
- [x] (optional) Implement a simple 2 phase-commit queue protocol with distributed locks using Boost Redis and redlock cpp.

### Instructions to Run and Setup

First clone this git repository using `git clone https://github.com/9401adarsh/ceph-redis-push-popper/`

On Local: 

You'll need to have the following pre-requisites to run the project on your local machine. 

- G++ >= V 11.0.0
- Redis Server >= V 6.0.0
- Boost Version >= 1.84.0
- OpenSSL, PThread and Crypto Libraris

Downloading Redis: On a Fedora based machine, use `sudo dnf install redis`

Downloading Other CPP dependencies - For Fedora machines, use `bash ./install-deps.sh` under the root folder of the project. 

To compile and get the object files: run `bash ./compile-scrips.sh` under root folder of the project. The object files should appear in the `/build` directory.

To run the files: 

- On a separate terminal, start redis using `redis-server`

- Running the object files, from project root is as follows

```
"Standard Redis Queue Usage"
echo "Usage: ./build/conusmer.o [HOST] [PORT] [Name of Queue] [optional: Number of Elements to Pop]"
echo "Usage: ./build/producer.o [HOST] [PORT] [Name of Queue] [list of elements to push]"

echo "2 PC Redis Queue Usage"
echo "Usage: ./buid/2pc-producer.o [optional argument for no locks to be used: noLocks]" 
echo "Usage: ./build/2pc-consumer.o [optional argument for no locks to be used: noLocks]"
echo "Ex: ./build/2pc-consumer.o noLocks (no locks for redis resource in this mode)" 

```

Running On Docker: 

Build the image using the Dockerfile at the project root. 

`docker build -t ceph-redis-queue .`

Start the container with the following command: 

`docker run --privileged -v /run/systemd/system:/run/systemd/system -v /var/run/dbus/system_bus_socket:/var/run/dbus/system_bus_socket -dit --name project-container ceph-redis-queue`

Open a terminal session in the container using

```
docker exec -it project-container bash
```

Inside the container session, now run the following commands

```
bash ./install-deps.sh
bash ./compile-files.sh
```
Meanwhile on another terminal, run this command to start the redis-server in the container.

```
docker exec -it project-container redis-server
```

Now on the previous terminal, running the object files, from project root is as follows

```
"Standard Redis Queue Usage"
echo "Usage: ./build/conusmer.o [HOST] [PORT] [Name of Queue] [optional: Number of Elements to Pop]"
echo "Usage: ./build/producer.o [HOST] [PORT] [Name of Queue] [list of elements to push]"

echo "2 PC Redis Queue Usage"
echo "Usage: ./buid/2pc-producer.o [optional argument for no locks to be used: noLocks]" 
echo "Usage: ./build/2pc-consumer.o [optional argument for no locks to be used: noLocks]"
echo "Ex: ./build/2pc-consumer.o noLocks (no locks for redis resource in this mode)" 
```

### 2 Phase Commit Queue - Basic Proof Of Concept

The 2PC Queue was devised based on the info provided here. 

#### Producer
- The producer deals with two Redis Queues - `topic:reserve` & `topic:commit`
- First a reservation made in `topic:reserve` using `LPUSH`. Then the RADOS operation    occurs
- If the operation was successful, we use `RPOPLPUSH` to move this notif to the `topic:commit`
- Else, we remove the reserved space on `topic:reserve` using `LREM`

#### Consumer
- The consumer only deals with a single Redis Queue - `topic:commit`
- Consumer only removes notifs from the queue when there are atleast 5 notifications on the queue - Removes it as a batch of 5 always, this is checked using `LRANGE`.
- The removal is done with `LREM` for each notification, if the notification is ACKED. 
- In case of NACK, the notification will remain in the queue to be processed later.

### An attempt at distributed locks

An attempt at distributed locks has been made by using the redlock-cpp library. [Repo Link](https://github.com/jacket-code/redlock-cpp)

The idea is that both consumers will accquire lock to the redis server in order for them to do the operations. 

For this attempt, I have used one Distribured Lock Manager which manages locks for the redis-server listening on `127.0.0.1:6379`. 

```
CRedLock *dlm = new CRedLock();
dlm->AddServerUrl("127.0.0.1", 6379);

#Acquiring the lock
CLock redisLock;
bool hasRedisLock = dlm->Lock("redisServer", 10, redisLock);

#Releasing the lock
dlm->Unlock(redisLock);
```

Code snippets for locks follow: 

`producer.cpp`

```
void producerThread(redis::config &cfg, CRedLock *dlm)
{

    std::string radosRequest = getRandomString();
    bool operationDone = false;
    while (operationDone != true)
    {

        bool reserveDone = false;
        bool commitOrAbortDone = false;

        CLock redisLock;
        bool hasRedisLock = dlm->Lock("redisServer", 10, redisLock);

        if (hasRedisLock)
        {
            asio::io_context reserveOpCtxt;
            reserveOperation(reserveOpCtxt, cfg, "topic:reserve", radosRequest);

            dlm->Unlock(redisLock);

            auto radosOutput = doRadosOperation(radosRequest);
            reserveDone = true;

            while (commitOrAbortDone != true)
            {
                hasRedisLock = dlm->Lock("redisServer", 10, redisLock);

                if (hasRedisLock)
                {
                    asio::io_context commitOrAbortOpCtxt;
                    commitOrAbortOperation(commitOrAbortOpCtxt, cfg, {"topic:reserve", "topic:commit"}, radosOutput);

                    dlm->Unlock(redisLock);
                    commitOrAbortDone = true;
                }
                else
                {
                    std::cout << "Waiting to get locks for doing commit" << std::endl;
                    sleep(1);
                }
            }
            operationDone = reserveDone && commitOrAbortDone;
        }
        else
        {
            std::cout << "Waiting to get lock for accessing redis server." << std::endl;
            sleep(1);
        }
    }
}

```
`consumer.cpp`

```
void consumerThread(redis::config &cfg, CRedLock *dlm)
{
    bool operationDone = false;
    while (operationDone != true)
    {

        bool reserveDone = false;
        bool commitOrAbortDone = false;

        CLock redisLock;
        bool hasRedisLock = dlm->Lock("redisServer", 10, redisLock);

        if (hasRedisLock)
        {

            asio::io_context eventThread;
            fetchNotification(eventThread, cfg, "topic:commit");

            dlm->Unlock(redisLock);
        }
        else
        {
            std::cout << "Waiting to get lock for accessing redis server..." << std::endl;
            sleep(1);
        }
    }
}
```

### Notes

- Until now, Redis doensn't offer the flexibility to lock only a particular resource. If we have to accquire a lock, we'll have to block the entire redis-server. This might cause an issue when we try to scale up. 

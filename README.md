## Evaluation Task 2 for Rados to Redis - Ceph GSOC 24

### To Do:
- [x] Implement a C++ client that pushes to a Redis Queue using Boost's Redis library.
- [x] Implement a C++ client that pops from a Redis Queue using Boost's Redis library.
- [ ] (optional) Write tests for the above two clients using GTest.
- [x] (optional) Implement a simple 2 phase-commit queue protocol with distributed locks using Boost Redis and redlock cpp.

### Instructions to Run and Setup

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



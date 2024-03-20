## Evaluation Task 2 for Rados to Redis - Ceph GSOC 24

### To Do:
- [x] Implement a C++ client that pushes to a Redis Queue using Boost's Redis library.
- [x] Implement a C++ client that pops from a Redis Queue using Boost's Redis library.
- [ ] (optional) Write tests for the above two clients using GTest.

### Running the CPP clients

The prerequisites for running the files are follows:
- C++ 17 and above
- Boost Version 1.84.0 and above
- Redis Version 6 and above
- G++ 11 and above

The files for push and pop can be found in the `src` folder.

Before running the files, you need to start redis.

```
sudo systemctl start redis
```

On a separate terminal, compile the clients separately as follows:

```
g++ redis-pusher.cpp -o redis-pusher -I /path/to/boost_1_84_0 -lpthread -lssl -lcrypto
g++ redis.popper.cpp -o redis-popper -I /path/to/boost_1_84_0 -lpthread -lssl -lcrypto
```

You can ignore the -I flag, if the default boost library in your system is of version 1.84.0 or higher.
  



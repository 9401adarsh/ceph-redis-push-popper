#!/bin/sh

appRoot=$PWD

targetDir=build
rm -rf "$targetDir"
mkdir $targetDir

cd ./src/redlock-cpp/hiredis && sudo make && sudo make install && cd "$appRoot"
cd ./src/redlock-cpp && make && cd "$appRoot"

g++ ./src/standardRedisQueue/producer.cpp -o ./build/producer.o -lpthread -lssl -lcrypto 
g++ ./src/standardRedisQueue/consumer.cpp -o ./build/consumer.o -lpthread -lssl -lcrypto

g++ ./src/2pc-redisQueue/producer.cpp -o ./build/2pc-producer.o -I ./src/redlock-cpp/ -L ./src/redlock-cpp/bin/ -lpthread -lssl -lcrypto -lhiredis -lredlock
g++ ./src/2pc-redisQueue/consumer.cpp -o ./build/2pc-consumer.o -I ./src/redlock-cpp/ -L ./src/redlock-cpp/bin/ -lpthread -lssl -lcrypto -lhiredis -lredlock

echo "Files Have Been Compiled and are present in $PWD/build"

echo "Standard Redis Queue Usage"
echo "Usage: ./build/conusmer.o [HOST] [PORT] [Name of Queue] [optional: Number of Elements to Pop]"
echo "Usage: ./build/producer.o [HOST] [PORT] [Name of Queue] [list of elements to push]"

echo "2 PC Redis Queue Usage"
echo "Usage: ./buid/2pc-producer.o [optional argument for no locks to be used: noLocks]" 
echo "Usage: ./build/2pc-consumer.o [optional argument for no locks to be used: noLocks]"
echo "Ex: ./build/2pc-consumer.o noLocks (no locks for redis resource in this mode)" 

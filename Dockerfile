FROM fedora:39 
WORKDIR /app
COPY . .

RUN sudo dnf install -y wget
RUN sudo dnf install -y g++
RUN sudo dnf install -y redis


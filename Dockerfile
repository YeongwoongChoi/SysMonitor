FROM sysmonitor-base

RUN ln -sf /usr/share/zoneinfo/Asia/Seoul /etc/localtime && \
    echo "Asia/Seoul" > /etc/timezone
    
WORKDIR /sysmonitor
COPY src/ src/
RUN mkdir -p bin
RUN gcc -o bin/server src/server.c

EXPOSE 8080/udp

CMD ["bash"]

FROM frolvlad/alpine-gcc

RUN apk add --no-cache make

COPY . /usr/src/myapp
WORKDIR /usr/src/myapp

RUN make

EXPOSE 8080
CMD ["./server"]

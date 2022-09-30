# Undefined Behavior Server 👀

**Undefined Behavior Server** is a HTTP 1.1 server made with **Epoll** and **Pthread** for the practice of programming in C. Actually it works quite well, I have tested it with a HTML template. According to **Valgrind** I don't have any memory leak `valgrind --leak-check=full bin/ubserver -a 127.0.0.1 -l`). Well, I lie, because in the logging library I wanted to try `aio_write`, to write asynchronously the logs and although I free the memory, Valgrind shows me some losses, but if I switch back to the previous function to write the logs, then I don't have any loss. I don't know why, [I will try to fix it later](https://github.com/chiqui3d/ub-server/blob/main/lib/logger/logger.c#L187).

At first it started as a single test with Epoll, but I have continued practising and finally got a small server serving static content with [epoll and pthread](https://github.com/chiqui3d/ud-server/blob/main/src/accept_client_thread_epoll.c). Currently, each thread has its own `epollFd`. But I have created a [branch](https://github.com/chiqui3d/ub-server/tree/epolloneshot) where the same epollFd is shared with all threads with the EPOLLONESHOT feature, but the event structure and priority queue is created in each thread, to not have race conditions.

The priority queue is made with the data structure min-heap, to manage the time of the connections and to be able to add the keep-alive feature, it is also good to close the connections that are not being used for a while, testing I have realized that Chrome does not close the connections until you close the browser.

https://github.com/chiqui3d/ub-server/blob/main/src/accept_client_epoll.c#L30

https://github.com/chiqui3d/ub-server/blob/main/src/queue_connections.c

I have also created a small library for logging and you can print the logs to a file if you wish. If you comment out the line of code in the Makefile containing `CFLAGS += -DNDEBUG`, you will be able to see the logs directly in the console instead of in a file. The logger writes to the file with `aio_write` function, so it is asynchronous and it does not block the main thread. [See options](#binubserver---help)

Currently, I have downloaded a free HTML template and put it directly into the `public` directory to test it out, and it seems to work quite well.

But as I said before, although it works, it still requires a lot of validation, and it is possible that it has some errors when dealing with some headers or options.

Some headers are added manually as can be seen in the code https://github.com/chiqui3d/ub-server/blob/main/src/response.c#L121, such as the cache header, which disabled the browser cache to avoid unexpected results during testing.

## Directory Structure

* **bin**: Contains the executable file of the server. These are generated once you compile the program.
* **build**: Contains the object files (.o) of the server together with their dependencies (.d) files.
* **include**: Contains the header files of the server.
* **lib**: Contains the libraries of the server. Trying to separate program code with other reusable code.
* **public**: Contains the static files of the server (html, css, js, images, etc).
* **src**: Source code of the server.
* **test**: Contains the test files of the server

## Dependencies
Currently the only dependency is the magic library to read the MIME type of a file. Installation is simple for Ubuntu:

```
sudo apt install libmagic-dev
```
Then include the flag for the compiler as you can see the MAKEFILE

```
-lmagic
```
The use of this library can be seen in:

    https://github.com/chiqui3d/ud-server/blob/main/src/response.c#L79


## Compilation/Installation

To compile the server you can use the MAKEFILE that is in the root of the project. The installation is done in one step, which is the compilation of the server, the generation of the executable file and running the program with the default options.

```
make
```
You can exit its execution with `CTRL + C` and execute again it with its options.

## bin/ubserver --help

```
Usage: bin/ubserver [ options ]

  -a, --address ADDR        Bind to local address (by default localhost)
  -p, --port PORT           Bind to specified port (by default, 3001)
  -d, --html-dir DIR        Open html and assets from specified directory (by default <public> directory)
  -l, --logger              Active logs for send messages to log file (by default logs send to stderr)
  --logger-path             Absolute path to directory (by default /var/log/ub-server/)
  -h, --help                Print this usage information

```
```
bin/ubserver -p 3001 -a 127.0.0.1 --logger-path /home/chiqui3d/www/CS50/c/sockets/server-html/
```
# Mention to
* @skeeto from the C_Programming Reddit community, who has corrected me and taught me many things. Besides, the only test that currently exists has been realized by him to explain and teach me how the server should respond when sending bytes at a time.

# TODO

* [x] Add support for HTTP keep-alive connections
* [x] Add pre-threaded (thread pool)
* [ ] URL decoding (e.g. %20 -> space)
* [ ] Check safe url directory. Example ../../etc/passwd
* [ ] Add support for Accept-Ranges 
     * https://www.rfc-editor.org/rfc/rfc9110.html#name-range-requests
* [ ] Add support for compression
     * https://www.rfc-editor.org/rfc/rfc9110.html#field.content-encoding
     * https://www.rfc-editor.org/rfc/rfc9112#section-6.1
* [ ] Look out for more optimizations [Institutional Coding Standard](https://yurichev.com/mirrors/C/JPL_Coding_Standard_C.pdf)
* [ ] Add support for HTTPS
* [ ] Add more tests
* [ ] Daemonize the server
* [ ] Decouple logging from main program


##  wrk -t2 -c100 -d30s http://127.0.0.1:3001/hello
Great results with a single Hello World, compared to the next test where i have to create the `struct Response`, load mime types, generate the `headers` dynamically (although some headers are hard coded) and `send` a large HTML file.

```
Running 30s test @ http://127.0.0.1:3001/hello
  2 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     9.93ms   22.07ms 240.83ms   91.83%
    Req/Sec     8.04k     5.30k   13.67k    68.49%
  433823 requests in 30.12s, 61.65MB read
Requests/sec:  14402.00
Transfer/sec:      2.05MB
```


##  wrk -t2 -c100 -d30s http://127.0.0.1:3001/index.html
```
Running 30s test @ http://127.0.0.1:3001
  2 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   136.52ms   98.13ms 784.54ms   73.47%
    Req/Sec   394.35     67.24   550.00     70.78%
  23549 requests in 30.09s, 345.92MB read
Requests/sec:    782.63
Transfer/sec:     11.50MB
```

##  wrk -t2 -c1000 -d30s http://127.0.0.1:3001/index.html
```
Running 30s test @ http://127.0.0.1:3001
  2 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   935.56ms  491.84ms   2.00s    61.66%
    Req/Sec   407.15     63.81   620.00     69.02%
  24117 requests in 30.07s, 354.27MB read
  Socket errors: connect 0, read 0, write 0, timeout 3478
Requests/sec:    802.13
Transfer/sec:     11.78MB
```

# References

* [Advanced Linux Programming book](https://mentorembedded.github.io/advancedlinuxprogramming/)
* https://www.reddit.com/r/C_Programming/
* https://stackoverflow.com/questions/tagged/c

I would like to read the following books, some of which I have used for reference, but have not completed.

* The C Programming Language by Brian W. Kernighan and Dennis M. Ritchie 
* The Linux Programming Interface
* Learn C the Hard Way
* Advanced Programming in the UNIX Environment
* UNIX Network Programming
# Undefined Behavior Server 👀

**Undefined Behavior** is the result I have had throughout the development and learning of this simple HTTP server in C. Currently I still have a lot more to apply, optimize and learn. Currently, according to **Valgrind** (`valgrind --leak-check=full bin/ubserver -a 127.0.0.1 -l`) I don't have any memory leaks..., but it is clear that a web server is quite complex, especially because of the amount of specifications it contains, along with the optimizations that this implies to support the maximum number of connections.

Currently, it is being developed with **Epoll** to accept client connections via event notifications, in which the file descriptors of each client/connection are included for later use. All this can be seen in https://github.com/chiqui3d/ud-server/blob/main/src/server_accept_epoll.c

My intention is to create another `server_accept_*.c` for `fork` and another for `thread` and test the differences with [wrk](https://github.com/wg/wrk), to learn along the way.

Currently, I have downloaded a free HTML template and put it directly into the `public` directory to test it out, and it seems to work quite well.

But as I said before, although it works, it still requires a lot of validation, and it is possible that it has some errors when dealing with some headers or options.

I would also like to reduce the number of times I use dynamic memory with `malloc`. As a newbie, I'm a bit of a mess when it comes to choosing one option or the other when assigning chars.

Someone posted on Reddit some good recommendations to follow in C [Institutional Coding Standard](https://yurichev.com/mirrors/C/JPL_Coding_Standard_C.pdf)

## Directory Structure

* **bin**: Contains the executable file of the server. These are generated once you compile the program.
* **build**: Contains the object files (.o) of the server together with their dependencies (.d) files.
* **include**: Contains the header files of the server.
* **lib**: Contains the libraries of the server. Trying to separate program code with other reusable code.
* **public**: Contains the static files of the server (html, css, js, images, etc).
* **src**: Source code of the server.
* **test**: Contains the test files of the server. Currently empty 😫

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

    https://github.com/chiqui3d/ud-server/blob/main/src/response.c#L52


## Compilation/Installation

To compile the server you can use the MAKEFILE that is in the root of the project. The compilation is done in one step, which is the compilation of the server, the generation of the executable file and it execution by default.

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
I am not very happy with these results.

```
Running 30s test @ http://127.0.0.1:3001/index.html
  2 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   495.92ms   37.06ms 547.16ms   95.64%
    Req/Sec   104.99     55.14   212.00     62.80%
  6006 requests in 30.10s, 88.32MB read
Requests/sec:    199.55
Transfer/sec:      2.93MB

```

# TODO

* [ ] Decouple logging from main program (thread and queue)
* [ ] Add support for keep-alive connections
* [ ] Add fork and thread alternative example
* [ ] Reduce the number of times i use dynamic memory with malloc
* [ ] Add support for compression
* [ ] Add more optimizations
* [ ] Add support for HTTPS
* [ ] Add tests

# References

* [Advanced Linux Programming book](https://mentorembedded.github.io/advancedlinuxprogramming/)
* https://stackoverflow.com/questions/tagged/c
* https://www.reddit.com/r/C_Programming/

I would like to read the following books, some of which I have used for reference, but have not completed.

* The C Programming Language by Brian W. Kernighan and Dennis M. Ritchie *(Initiated)*
* The Linux Programming Interface *(Initiated)*
* Learn C the Hard Way *(Initiated)*
* Advanced Programming in the UNIX Environment
* UNIX Network Programming
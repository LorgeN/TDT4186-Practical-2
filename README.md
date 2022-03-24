# TDT4186 Operating Systems - Practical Exercise 2

Contains solution for assignment made by group 80

## Building

Simply run `make` to build the project. The compiled program will be located at `./mtwwwd`. You can run it using `./mtwwwd`. You may also use `make clean` to clean any generated build files.

## Running

Usage: `./mtwwwd <www-path> <port> <worker_threads> <buffer_slots>`

## Testing with curl

### IPv4

```bash
 curl --http0.9 localhost:<port>/lorgs.html
```

### IPv6

```bash
curl --http0.9 -6 http://\[::1\]:<port>/lorgs.html
```

### Useful commands

View system calls like accept, close etc.

```bash
strace -f -e trace=close,accept4,accept,pipe,open ./mtwwwd ./src/www 6970 8 16
```

## Exploit (Task e)

This server currently features no verification that the file request is actually within the working directory specified in the command line. For example,

```bash
curl --http0.9 --path-as-is http://localhost:<port>/../../README.md
```

will work just fine, if you consider fine as returning the specified file without complaint. Note the `--path-as-is`, this is required for curl not to remove the `..` in the file path. Using this we can access files that are not within the server's working directory (the directory we wish to publish through the web server).

There are several options for patching this exploit. The simplest is probably just to check the file path for any `../` or similar, and return some error if it does contain this. One could also look into something like [chroot](https://man7.org/linux/man-pages/man2/chroot.2.html), and use that to create a form of "jail" for the process.

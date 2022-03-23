# TDT4186 Operating Systems - Practical Exercise 2

Contains solution for assignment made by group 80

## Building

Simply run `make` to build the project. The compiled program will be located at `./mtwwwd`. You can run it using `./mtwwwd`. You may also use `make clean` to clean any generated build files.

## Running

Usage: `./mtwwwd <www-path> <port> <worker_threads> <buffer_slots>`

## Testing with curl

```bash
 curl --http0.9 localhost:6969/lorgs.html
```

 Also note that this server currently features no verification that the file request is actually within the working directory specified in the command line. For example,

 ```bash
 curl --http0.9 --path-as-is http://localhost:6970/../../README.md
```

will work just fine, if you consider fine as returning the specified file without complaint.
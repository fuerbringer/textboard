# /c/chan

A simple textboard server written entirely in C.

An instance is gratuitously hosted by @Hoi15A: [chan.neat.moe](https://chan.neat.moe/)

## But... but why?

That's classified!

## But there's a python file right there!

The python script (`enquote.py`) is just used to generate a header file with all the static files for use in C.

If you don't like it, feel free to write your own header file.

## Hacking

### How do I compile/execute it?

1. Clone this directory.

2. Get a working C compiler (`gcc`, `clang`), coreutils and python3.

3. If you're confident that this one man project wouldn't segfault, run `make PRODUCTION=1`, else just do `make`.

4. Then just run it!

If you receive a segfault or some error in the server side, feel free to make an issue with the stack trace found in `gdb`.

### But muh containerization!

There's a `dockerfile` for that.

### Directory structure

```
  textboard
   \_ src
     \_ config.h
   \_ static
   \_ database.csv
   \_ Makefile
   \_ server
```

`src` is where the C files are located.

`src/config.h` is the configuration file for the server.

`static` is where the HTML/CSS files are located.

Files in the `static` directory should be of the name `VAR.ext`, where `VAR` is the name of the macro containing the source of that file, and `ext` is the extension.

`database.csv` is the database of the textboard server.

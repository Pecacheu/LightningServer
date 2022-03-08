# LightningServer
High performance C++ HTTP/HTTPS web server based on [LightningHTTP](https://github.com/Pecacheu/LightningHTTP).

## Usage Instructions

Run with `./LightningSrv`
Your HTTPS certs should be located at `web.crt` and `web.key`, files go in `./web`
However, all of this is customizable! Have a look at the code, there's lots of comments.

## Build Instructions for Noobs

Step 1: Be on Linux. If you're using an uncommon distro or a non-Linux OS and it won't compile, just submit and issue and I'll have a look. Alternatively, if you **aren't** using a Unix-based OS to host your server, I can suggest several local asylums.

You'll need to install a few tools to get going. On most Debian-based distros you do this using **apt**, which is *totally* a great package manager and has *no flaws whatsoever.* If you are using apt, all you need to do is run `sudo apt install gcc g++ libssl-dev zlib1g-dev`

Finally, compile the server with `bash compile.sh`
There are several *optional arguments,* too. `bash compile.sh http` compiles only the http library and not utils. `bash compile.sh server` compiles only the server and not any of the libraries. Adding the `debug` option to the end of any of these compiles with debug mode on, allowing you to test with a debugger like `gdb`.

Now the astute among you may ask, why not just use *make* for this instead of an overkill bash script with argument parsing? **Good question!**

### Standard Dependencies:
- C++17 & std library (w/ std::thread)
- std::fstream (File read/write)
- C++ filesystem (General fs access. Could use boost.filesystem)
- Unix sys/inotify.h (Tracks filesystem changes)
- Unix sys/socket.h
- *Optional:* sys/utsname.h

### Library Dependencies:
- LightningHTTP v3.6.2 or later
- C-Utils v2.2 or later
- OpenSSL v1.1 or later
- *Optional:* Zlib (File compression)
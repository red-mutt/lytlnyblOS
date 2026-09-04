# Part XIII: The Shell and Programs

# What are we making?

This is the final part of the guide. You deserve a pat on the back for coming this far.
This chapter will end in us making a program called a shell. 
Different kinds of shells for different operating systems can 
do many different things, but, a shell is a 
user space program that lets you interact with the computer by typing commands.

Our shell will have the following functionality:
-   `ls [directory]` to list the entries in a directory.
-   `mkdir [path]` to make a new directory in the file system.
-   `touch [path]` to make a file in a directory.
-   `rm [path]` to delete files and directories.
-   `run [path]` to run programs stored within the file system.
-   `cat [path]` to read from files.
-   `write [path]` to read to files.

This is all I will be instructing you on how to implement, but this is another part where
you can become creative and make your own commands.

There may be confusion in the `run` command. "How can we get files to run
when there is no way to store them in the file system?" This is where
`MKFS` comes in. 

`MKFS` means "make file system" and is a piece of software we will be writing
for the host operating system. It will copy all the files and directories in a 
special directory on the host operating system (called `rootfs`) to the file system
that our custom operating system will be using. The compiled binaries for user space programs
are then stored somewhere within `rootfs` which are then be ready to be loaded 
into memory from the file system in the custom OS.

Let's get into writing the MKFS.


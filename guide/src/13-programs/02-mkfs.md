# MKFS

## Why do we need this

One of the largest components of the shell is its ability to load
programs from the filesystem into memory. Before we work on loading from
the file system into memory, we need to get files into the filesystem somehow.
This is where MKFS (make filesystem) comes in.
In our host OS, we may make a directory like this:

```
rootfs
├── bin
│   ├── user_test
│   └── shell
└── documents
    └── notes.txt
```

MKFS copies all the directories and files in `rootfs` to the filesystem we just made.
This is the reason we split `fs_layout.h` and `fs.h`. `fs_layout.h` will get included within
the code for MKFS to get the layout and structures for the filesystem.

A lot of the code for our MKFS will be copied from the core of our file system.
This is because the MKFS essentially does 3 separate things:

-   Format the file system 
-   Copy data from `rootfs`
-   Write and read data to `kernel.img`.

Code for formatting the file system and writing and reading form `kernel.img` already exists,
so the only added complexity comes from traversing `rootfs` in the host operating system and
copying to memory.

# Part XII: Simple File System

## What do we have now

We made our bootloader a while ago, if you remember, in our
bootloader we were reading from a disk (this disk being the `kernel.img`
disk image) and loading the data directly into memory at `0x9000`. What
we have right now is not a file system, although we did 
interface with some sort of disk or secondary storage.

We can keep this primitive method of loading the kernel, we 
will not make the file system responsible for loading our kernel
into memory for our operating system (although you can). 
Our bootloader can continue using the primitive method I
just described. Just like all our other technologies, the file system
can be initialized and access the rest of the disk once the storage driver 
is available.

We don't have a file system, but what we do have is a (virtual) disk
that is a part of our virtual machine that we
have left untouched (other than in the bootloader). On our disk
there is a structure such as:
`[bootloader][kernel][kernel][kernel]...` The file system we make is
essentially a way we are going to interpret and communicate with the
data on this disk. For example, we may have:

```
Sector 0
    bootloader

Sectors 1-50
    kernel (and future additions to the kernel...)

Sector 51
    filesystem superblock

Sectors 52-60
    free-space bitmap

Sectors 61-100
    inode table

Sectors 101+
    data region
```

At this state, the first Makefile setup I showed you in our first few chapters
made the `kernel.img` disk image only have enough space to contain the bootloader and 
the kernel, so you will have to expand this with `dd` command.

## What should we make?

### The filesystem

We are making a block based, inode based, semi Unix like file system. It
will be similar to file systems such as: `ext2`, `ext3` and `ext4`.

The disk would just be a sequence of storage units called blocks. Each
block would be 512 bytes long and something like a text file
may occupy a certain number of blocks. We need a way to track the
relationship between blocks and files, this is where inodes come in.

An inode would essentially be the file system's record describing files.
An inode would contain information such as: file type, file size, file
permissions, ownership, timestamps, pointers to blocks.

You may then notice that when it comes to inodes, we didn't mention the
filename, this is because a filename would be stored within a directory,
a directory essentially is a mapping between a filename and it's
respective inode. When we access `"/hello.txt"` our filesystem does:
`"/hello.txt" -> root directory (find "hello.txt") -> inode x (find data blocks -> found blocks y and z -> parse file contents`

We also mentioned a superblock, this 
is metadata about the whole file system, ours may contain the:
type, size of each block, block count, location of inode table, location
of free-space map, location of data region. Without the superblock, we
would have to assume a lot of things about our file system, which is not
perfect practice.

What's the free-space bitmap? This is essentially the same thing we did
within our physical memory manager but for the file system, it describes
what blocks are occupied and which aren't within a simple bitmap.

And then finally we have the data region, where our blocks are actually
contained.

### The drivers

When we were in our BIOS, `int 13h` allowed us to
communicate with our disk. But here, we will have to write our own
driver to communicate with the disk. QEMU can emulate many
different types of storage devices and controllers, but we will be
writing our driver for the ATA PIO. ATA and PIO are two different
things, but are a combination used to make up our whole driver.

After our driver is created, we will start writing the code for our
file system as we described above. If you then want to make new drivers
for new types of storage devices, it will be easy to do so due to 
 the file system being abstracted from the driver.

### The manager

When I walk you through the implementation of the filesystem, you will see 
that it can quickly become a maze of about 20 functions that all interact with
another and can become confusing to navigate. Due to this, I will make a file system manager
this will include functionality like `fs_open()`, `fs_close()`, `fs_read()`, `fs_write()`.
I will also make `ls`, `mkdir`, `touch`, `rm`, these would typically be their own 
user space programs, but I'm going to be embedding them into the file system manager, so our
file system becomes easier to use earlier on.

## How do we make this?

First the driver, next the file system, then a manager for the filesystem.




-DEV NOTES- 
-remember to write about updates to the Makefile (for making a larger disk and for general organisation purposes) 
-write about the fs_manager too 


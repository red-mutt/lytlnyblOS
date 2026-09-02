# Part XII: Simple File System

## About file systems

### What do we have now

We made our bootloader a while ago, if you remember, within our
bootloader we were reading from a disk (this disk being the `kernel.img`
disk image) and loading the data directly into memory at `0x9000`. What
we have right now is not a file system, i just wanted to make that
clarification now because that's the only point in our development
where we had to interface with some sort of disk or secondary storage.

We can also keep this primitive method of loading the kernel, we don't, 
and we will not make the file system responsible for loading our kernel
into memory for our operating system (although you can by yourself if
you want to). Our bootloader can continue using the primitive method I
just described. Just like all our other technologies, the file system
can be initialized and access the rest of the disk.

We don't have a file system, but what we do have is a (virtual) disk
that is a part of our virtual machine throughout all execution that we
have left untouched (other than in the bootloader). And on our disk
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

### What should we implement

We are making a block based, inode based, semi Unix like file system. It
will be similar to file systems such as: `ext2`, `ext3` and `ext4`.

The disk would just be a sequence of storage units called blocks. Each
block would be 512 bytes long and therefore something like a text file
may occupy a certain number of blocks. We need a way to track the
relationship between blocks and files, this is where inodes come in.

An inode would essentially be the file system's record describing files.
An inode would contain information such as: file type, file size, file
permissions, ownership, timestamps, pointers to data blocks.

You may then notice that when it comes to inodes, we didn't mention the
filename, this is because a filename would be stored within a directory,
a directory essentially is a mapping between a filename and it's
respective inode. When we access `"./hello.txt"` our filesystem does:
`"./hello.txt" -> rootdir (find "hello.txt") -> inode x (find data blocks -> found blocks y and z -> parse file contents`

We also mentioned a superblock, this is what describes the file system
itself. It's metadata about the whole file system, ours may contain the:
type, size of each block, block count, location of inode table, location
of free-space map, location of data region. Without the superblock, we
would have to assume a lot of things about our file system, which is not
perfect practice.

What's the free-space bitmap? This is essentially the same thing we did
within our physical memory manager but for the file system, it describes
what blocks are occupied and which aren't within a simple bitmap.

And then finally we have the data region, where our blocks are actually
contained.

### How do we implement this

When we were in our BIOS, we had `int 13h` that allowed us to
communicate with our disk. But here, we will have to write our own
driver to communicate with the disk. QEMU can emulate many
different types of storage devices and controllers, but we will be
writing our driver for the ATA PIO. ATA and PIO are two different
things, but are a combination used to make up our whole driver.

After our driver is created, we will start writing the code for our
file system as we described above. If you then want to make new drivers
for new types of storage devices, it will then be easy to do so, as the
file system will be made to be abstracted from the driver, so you can
easily swap out drivers with minimal repercussions and refactoring
required.

-DEV NOTES- 
-remember to write about updates to the Makefile (for making a larger disk and for general organisation purposes) 
-write about the fs_manager too 


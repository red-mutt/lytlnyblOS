# Part XII: Simple Filesystem

## About filesystems

### What do we have now

We made our bootloader quite a while ago, if you remember, within our
bootloader we were reading from a disk (this disk being the kernel.img
disk image) and loading the data directly into memory at `0x9000`. What
we have right now is not a file system, i just wanted to make that
clarification now becuase that\'s the only point in our development
where we had to interface with some sort of disk or secondary storage.

We can also keep this primitive method of loading the kernel, we don\'t
and we will not make the filesystem responsible for loading our kernel
into memory for our operating system (although you can by yourself if
you want to). Our booloader can continue using the primitive method i
just described. Just like all of our other technologies, the filesystem
can be initialised and access the rest of the disk.

So we don\'t have a filesystem, but what we do have is a (virtual) disk
that is a part of our virtual machine thoughout all of execution that we
have left untouched (other than in the bootloader). And on our disk
there is a structure such as:\
`[bootloader][kernel][kernel][kernel]...` The file system we make is
essentially a way we are going to interpret and communicate with the
data on this disk. For example, we may have:

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

### What should we implement

We are making a block based, inode based, semi unix like filesystem. It
will be relatively similar to filesystems such as: ext2, ext3 and ext4.

The disk would just be a sequence of storage units called blocks. Each
block would be 512 bytes long and therefore something like a text file
may occupy a certian number of blocks. We need a way to track the
relationship between blocks and files, this is where inodes come in.

An inode would essentially be the filesystem\'s record describing files.
An inode would contain information such as: file type, file size, file
permissions, ownership, timestamps, pointers to data blocks.

You may then notice that when it comes to inodes, we didn\'t mention the
filename, this is becuase a filename would be stored within a directory,
a directory essentially is a mapping between a filename and it\'s
respective inode. So when we access \"./hello.txt\" our filesystem does:
`"./hello.txt" -> rootdir (find "hello.txt") -> inode x (find data blocks -> found blocks y and z -> parse file contents`

We also mentioned a superblock, this is what describes the filesystem
itself. It is metadata about the whole filesystem, ours may contain the:
type, size of each block, block count, locaiton of inode table, location
of free-space map, location of data region. Without the superblock, we
would have to assume a lot of things about our filesystem, which is not
really good practice.

What\'s the free-space bitmap? This is essentially the same thing we did
within our physical memory manager but for the filesystem, it describes
what blocks are occupied and which aren\'t within a simple bitmap.

And then finally we have the data region, where our blocks are actually
contained.

### How do we implement this

When we were in our bios, we had `int 13h` that allowed us to
communicate with our disk. But here, we will have to write our own
driver in order to communicate with the disk. QEMU can emulate many
different types of storage devices and controllers but we will be
writing our driver for the ATA PIO. ATA and PIO are two different
things, but are a combination used to make up our whole driver.

After our driver is created, we will start writing the code for our
filesystem as we described above. If you then want to make new drivers
for new types of storage devices, it will then be easy to do so, as the
file system will be made to be abstracted from the driver, so you can
easily swap out drivers with minimal reprocussions and refactoring
required.

-DEV NOTES- 
-remember to write about updates to the makefile (for making a larger disk and for general organisation purposes) 
-write about the fs_manager too 


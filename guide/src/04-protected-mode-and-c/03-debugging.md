## Debugging our issue

Let's add our flags back and debug with GDB, when debugging with GDB we
will sometimes want to see the next instructions using the program counter.
In real mode we have to account for segment base when calculating the 
physical address. (I'm pretty sure this is only a quirk with real mode 
by the way). `x/20i $pc` (which shows the next 20
instructions) should become: `x/20i (($cs * 16) + $pc)` I have made a
`.gdbinit` script to access these commands easier:

    set architecture i8086

    display/i (($cs * 16) + $pc)

    define xi
        x/20i (($cs * 16) + $pc)
    end

    define sii
        si
        x/10i (($cs * 16) + $pc)
    end

Now when we run the kernel we should see "Guest has not initialized the
display." Before we connect we must run the `gdb` command to go into
GDB. Then do `file kernel.elf` to load our labels,
`source .gdbinit` to load our GDB init file,
and finally `target remote localhost:1234 ` to connect to QEMU.
Everything is set up now. Set a breakpoint at `start` and type `c` to
continue execution until we hit our breakpoint. We can then try the `xi`
command I have made to see our next instructions. They should match 
our code.

You can set a breakpoint at our `enter_protected` label and then step
towards our jump, you'll see it will jump to an
unintended point, so something is going wrong. Take a look around, I'll
give you some commands that will be useful for GDB, and then 
I'll give you the solution.

## Some useful GDB commands

(Addresses and registers I put here are placeholders to represent
commands and not specific to check for our problem)

### Printing registers

```
info registers

print/x $eax
print/x $eip 
```

The first shows information about all registers. The last two
show information about specific registers.

### Disassemble instructions/functions

```
x/20i $pc 

x/20i 0x7C00

x/20i p_mode_main
```

The first prints instructions at current location indicated by the
program counter, second does at an address, third does it at a label.

### Viewing raw memory

```
x/16bx 0x7C00
x/16hx 0x7C00
x/16wx 0x7C00
```

Views raw memory, useful when we aren't sure if GDB is decoding our
instructions correctly. We can check reference manuals to make sure
memory is represented how we want it to. First does bytes, second
does words, third does double words.

### Breakpoint stuff

```
break 0x7C00
break p_mode_main
info breakpoints 
delete 1
```

This is how we make, get information about, and delete breakpoints.

### Watching execution

```
display/i $pc 
display/x $eax
```

These output registers after each step (`si`) command.

### Find the error!

You are now equipped to find the error. The next piece of text will
showcase how to find the solution. I suggest you try to find it yourself
a bit before you look at my solution, being proficient with debugging is
an important skill as I've said before

## Solution to our triple fault

In GDB, if we make a breakpoint at `enter_protected`, and then we go
right before our `lgdt` command and use:
`(gdb) x/8bx (($cs * 16) + $pc)`to see raw memory, we will see this
output:

```
0x9019 <enter_protected+1*>:   0x0f    0x01    0x16    0x8e    0x90    0x0f    0x20    0xc0
```

`0xf 0x1 0x16` is the opcode for our `lgdt` instruction. `0x8e 0x90` is our 
operand, which decodes to `0x908e` (with the other bytes being the next instruction).
`0x908e` is the address of `gdtr`, but this is not how we are supposed to use `lgdt`.
In real mode this address is interpreted as an offset from the `DS` 
segment base. Our code is loaded at physical address `0x9000`, so we need to use 
the offset of `gdtr` relative to start instead. We much change our
instruction to:

```x86asm
lgdt [gdtr - start] ; load GDTR with the GDT's address and size
```

We may also notice that we are not printing correctly too, this is the
same issue, so let's change that too:

```x86asm
mov si, hello_string - start
```

We should now be in protected mode! Another good debugging technique
is checking other people's implementations. That's how I originally
solved this issue, but it's also solvable via GDB. If you're thinking
"How could I even possibly realize that" Then welcome to bare metal
programming :)


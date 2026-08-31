## Debugging our issue

Let's add our flags back and debug with GDB, when debugging with GDB we
will sometimes want to see the next instructions using the program
counter, but this will not work as we have to factor in the address
specified by the code segment. (I'm pretty sure this is only a quirk
with real mode by the way). `x/20i $pc` (which shows the next 20
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

Now when we make we should see "Guest has not initialized the
display", before we connect we must run the `gdb` command to go into
GDB then we must do `file kernel.elf` to load our labels and such, and
then `source .gdbinit` to load our GDB init file, we should
then do `target remote localhost:1234 ` to connect to QEMU,
everything is set up now. Set a breakpoint at `start` and type `c` to
continue execution until we hit our breakpoint. We can then try the `xi`
command I have made to see our next instructions, they should match with
our code.

You can set a breakpoint at our `enter_protected` label and then step
towards our jump, and you'll see it will jump to an
unintended point, so something is going wrong, take a look around, I'll
give you some commands that will be useful for GDB and then after that
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

first states info of all registers, last two show info of specific
registers

### Disassemble instructions/functions

```
x/20i $pc 

x/20i 0x7C00

x/20i p_mode_main
```

The first prints instructions at current location indicated by the
program counter, second does at an address, third does it at a label

### Viewing raw memory

```
x/16bx 0x7C00
x/16hx 0x7C00
x/16wx 0x7C00
```

Views raw memory, useful when we aren't sure if GDB is decoding our
instructions correctly, we can check reference manuals to make sure
memory is represented how we want it to. First does bytes, second
does words, third does double words.

### Breakpoint stuff

```
break 0x7C00
break p_mode_main
info breakpoints 
delete 1
```

This is how we make get info about and delete breakpoints.

### Watching execution

```
display/i $pc 
display/x $eax
```

These output registers after each step (`si`) command.

Now you are equipped to find the error, the next piece of text will
showcase how to find the solution, I suggest you try to find it yourself
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

`0xf 0x1 0x16` is the opcode for our `lgdt` instruction `0x8e 0x90` is our
operand which decodes to `0x908e` (and the other stuff being the next
instruction). This *IS* the address of GDTR, but this is not how we are
supposed to use `lgdt`, as we are supposed to use an offset as our code
segment is set to `0x9000` at the `start` label. We much change our
instruction to:

```x86asm
lgdt [gdtr - start] ; load GDT registor with start address of GDT
```

We may also notice that we are not printing correctly too, this is the
same issue, so let's change that too:

```x86asm
mov si, hello_string - start
```

We should now be in real mode! Another good debugging technique
is checking other people's implementations, that's how i originally
solved this issue. But it's also solvable via GDB. If you're thinking
"How could I even possibly realize that" Then welcome to bare metal
programming :)


# x86 Run-Time Stack

I'll assume you already know how the stack data structure works
in its usual context, as it's one of the most basic data
structures in computer science, if you don't, do not worry, as it's
super simple and there are many great explanations
[online](https://www.geeksforgeeks.org/introduction-to-stack-data-structure-and-algorithm-tutorials/)

## The implementation

The reason we need the run-time stack is to manage the lifecycle of
functions, so what happens when we call and return from a function,
etc.? We know that a program consists of many subroutines
(functions), and all these subroutines fulfil a specific goal
within the program. Let's say a function `y` starts its life when 
called by another function, `x`. `y` is the callee, and `x` is the caller.
The callee can define its own local variables, which are
private, and these variables can be removed from memory once the callee
returns.

When the callee returns, the processor needs to know when it
finishes and the location of the code that we should return to (which is
right after the function call)

Typically, each process will have its own run-time stack; this is a
memory region that obeys the rules of the stack data structure. The 
run-time stack is divided into multiple sections called "stack 
frames." Each stack frame relates to a function that has been called 
during execution. Once the function ends, its stack frame can be 
removed from the run-time stack.

The register EBP (base pointer) typically contains the address used
as the base of the current stack frame, the register
`ESP` contains the memory address of the top of the stack. To push 
a new item to the stack, the push instruction can be used,
with the operand being the value to push. In our 32-bit stack,
this decrements `ESP` by 4 and stores the value at the new top 
of the stack; this means that the value of the x86 run time 
stack grows down in memory.

The `pop` instruction can also be used; this will increment the
value of the ESP and store the value that was on the top of the stack to
the memory location or register specified by the operand. It also won't
overwrite the popped value with null data; it will just leave it. This
means the old value remains in memory until that memory is overwritten
by something else.

### The `cdecl` calling convention

When a function needs to call another, the caller should push all 
the parameters that should be passed to the callee onto the stack.
The callee's parameters will be on the caller's stack frame. It's also worth
noting that parameters are pushed in reverse order, so parameter 1 will 
have a lower memory address than parameter 2 on the stack. After this, the
`call` instruction is used to start running the code of the other
function, but before jumping, the instruction pushes the return address onto
the stack. In 32-bit mode, this is the address of the instruction after the call,
which is later loaded into `EIP when we return, 
this is done so we know where to go next after we are
done executing our code.

When a function starts in a program, it's responsible to create its
own stack frame, so the first thing a function should have in its code
is the code that makes a new stack frame. A function can do this by pushing
the current value of `EBP` onto the stack; this is because `EBP` is going to be 
changed in a second. Then we move the value of `ESP` (stack pointer) to `EBP`,
making `EBP` the base of the current stack frame.
Now, we can continue working with our function, and it's stack pointer
now, pushing any value we may need, etc.

You may be thinking, "But how do we reach our parameters if they are
further down the stack?" Well, this is where the EBP register comes in,
as we can use this to access our parameters by referencing an
incremented value of it, which is always the same in this stack-frame layout,
as only the prior `EBP` and return address are stored between the current `EBP`
and the parameters.

It's also worth noting that the values we push in our examples are 4
bytes, as we are using 32-bit operands in 32-bit protected mode. 
To reach the first parameter using `EBP`, we use `[EBP + 8]`, where 
the offset is measured in bytes. You may think we need to add 12 to reach
the starting address of the first parameter. However, the saved `EBP` takes
up 4 bytes at `[EBP]`, and the return address takes up another 4 bytes at
`[EBP+4]`, so the first parameter starts at `[EBP+8]`.

When the callee needs to return any sort of value, we can just store it
in a register like EAX, and then to actually return, we first restore the 
previous stack frame by moving `ESP` back to `EBP` and then popping the saved
`EBP` value. Then at the top of the stack is the return address. The 
x86 instruction called `RET` can be used to return; It pops this
address from the stack and loads it into EIP.

Then, when our caller gets control again, we can remove the parameters
from the stack to reclaim the space. You might expect to use `pop`, but we
can instead increment the stack pointer by 4 for each 4-byte parameter.

The implementation of calling and returning from functions is not
written in stone for x86; it is simply a convention; this one is known
as `cdecl`, or the C declaration calling convention. Many other conventions exist, and
you can even make/design your own.

### Growth direction of the Stack

When I state that a stack is growing downwards, this simply means that
the newer items being added to the stack have smaller memory addresses
than the prior ones. The bottom of the stack has a larger memory address
than the top of the stack when the stack is growing downwards.

The x86 stack grows downwards: when values are pushed onto the stack,
`ESP` decreases, and when values are popped, `ESP` increases.
You can design a software stack that grows upwards, but the x86 `PUSH`
and `POP` instructions themselves use the downward-growing stack
convention. You may remember the expansion-direction flag I left out
when we covered stacks. This flag does not control whether the stack grows
upwards or downwards; instead, it determines whether a data segment is an expand-up 
expand-down segment.

### Advantages and disadvantages of growing upwards and downward

> [NOTE]

Downwards stack growth has been widely used for several reasons.
One possible historical reason is that early computers had limited memory,
so it was useful for the stack to grow into available space rather than 
requiring a large fixed allocation. After this, architectures would have had
reasons to maintain compatibility with previous versions of the same architecture.

An upwards-growing stack can also be resized as needed; the direction
itself does not determine whether a stack can be resized efficiently. 
Whether the stack and heap grow towards or away from each other is a 
separate design choice. But I haven't talked about the
heap that much, so don't really worry about it.


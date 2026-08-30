## x86 Run-Time Stack

I'll assume you already know how the stack data structure works
regularly without a context as it's one of the most basic data
sructures in computer science, if you don't, do not worry, as it's
super simple and there are many great explanations
[online](https://www.geeksforgeeks.org/introduction-to-stack-data-structure-and-algorithm-tutorials/)

### The implementation

The reason we need the run time stack is to implement the lifecycle of
functions, so what happens when we call and return from a function,
etc.? We know that a program usually consists of many subroutines
(functions), and all of these subroutines fill out a specific goal
within the program. Let's say a function called *y* starts it's life
when called by another function: *x*, *y* is the callee, and *x* is the
caller. The callee can define its own local variables, which are
private, and these variables can be removed from memory once the callee
returns.

So, when the callee returns, the processor needs to know when it
finishes and the location of the code that we should return to (which is
usually right after the function call)

In x86, each process will have it's own run-time stack; this is a
memory region that obeys the rules of the stack data structure. The run
time-stack is divided into multiple small stacks; these are called
"stack frames." Each stack frame relates to a function that has been
called during execution, once the function ends it is removed from the
larger run-time stack, therefore removing it from memory.

The register [EBP]{.hljs} (stack frame base pointer) contains the
starting memory address of the current stack frame, the register
[ESP]{.hljs} contains the memory address of the top of the stack. To
push a new item to the stack, the [push]{.hljs} instruction can be used,
where the operand is the new item. will decrement the value of ESP to
get a new starting address for the next value; this means that the value
of the x86 run time stack grows down in memory.

The [pop]{.hljs} instruction can also be used; this will increment the
value of the ESP and store the value that was on the top of the stack to
the memory location. or register specified by the operand. It also
won't overwrite the popped value with null data; it will just leave it.
This is for performance, as writing a bunch of 0's in the place would
take up unnecessary recourses. when we can just change the existing data
when overwriting in the future.

#### The cdecl calling convention

When a function needs to call another, the caller should push all the
parameters that should be passed to be into the callee to the stack. So,
the callee's parameters will be on A's stack. frame. It's also worth
noting that parameters are pushed in revered order, so parameter 2 will
appear below parameter 1 on the stack frame. After this, the
[call]{.hljs} instruction is used to start running the code of the other
function, but before jumping, the instruction pushes the value of the
return address onto the stack, which would be stored in the program
counter (EIP), this is done so we know where to go next after we are
done executing our code.

When a function starts in a program, it is responsible to creating its
own stack frame, so the first thing a function should have in it's code
is the code that makes a new stack frame. A function does this by
pushing the current value of EBP to the stack frame; this is because
it's going to be changed in a second. then we move the value of ESP
(stack pointer) to EBP (starting address of the current stack frame). So
now, we can continue working with our function and it's stack pointer
now, pushing any value we may need, etc..

You may be thinking, "But how do we reach our parameters if they are
further down the stack?" Well, this is where the EBP register comes in,
as we can use this to access our parameters. by referencing an
incremented value of it, which would always be the same as only the
prior EBP and return address are stored between the current value of the
EBP and the required parameters.

It's also worth noting that all values pushed to the stack are worth 4
bytes, as we are in 32-bit protected mode. So to reach a parameter using
EBP, we can do EBP + 8 as the value of the EBP, which moves in bytes.
You may think we need to add 12 to reach the starting address of the
first parameter. but remember the stack grows downwards, and the data
would be stored upwards.

When the callee needs to return any sort of value, we can just store it
in a register like EAX, and then to actually return, we can just pop all
the values from the stack. until we get to the prior value of EBP, this
can then be popped into the actual EBP register. Then at the top of the
stack is the memory address we need to return to, So this can just be
loaded into the program counter; this can be done using the x86
instruction called [RET]{.hljs} (which will also pop the value so we
will not need to pop it ourselves.)

Then, when our caller gets control again, we can pop our previous
parameters, just so we can clear up some memory space for the remainder
of the function. You would expect to use pop. But it can be done just by
incrementing the stack pointer by 4, and this deallocates the memory
without having to store it somewhere (as pop stores the values and
removes them).

The implementation of calling and returning from functions is not
written in stone for x86; it is simply a convention; this one is known
as cdecl, or the C declaration. There are many other conventions, and
you can even make/design your own.

#### Growth direction of the Stack

When I state that a stack is growing downwards, this simply means that
the newer items being added to the stack have smaller memory addresses
than the prior ones. The bottom of the stack has a larger memory address
than the top of the stack.

The stack growing downwards is done by default in x86; there are also
visualisations where the stack grows upward that are used in other
systems. The reason why stacks usually grow downwards with x86, which is
also probably due to historical design factors, but you could actually
make the x86 run time stack grow upward if you wanted to, but for now,
I'll just stick with letting it grow downwards; you may remember the
expansion-direction flag I left out for when we covered stacks. Well,
when it's value is 0, the stack will grow downwards, and to grow
upwards, it's value should be 1.

#### Advantages and disadvantages of growing upwards and downward

Downwards stack growth is mostly used because of many reasons, the most
prominent being from early computing when memory was limited, so it
would not be good to pre-allocate a large chunk of memory to exclusively
be used by the stack, down growing fixed this, providing an efficient
use of memory. After this, most architecture just would have maintained
compatibility with previous version of the same architecture.

The upwards growing stack allows us to resize the stack, at the cost of
inefficient memory usage (this also has to do with the stack and the
heap growing in opposite directions). But I haven't talked about the
heap that much, so don't really worry about it.


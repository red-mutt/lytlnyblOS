# Part VI : Interrupts & IDT

## What are we doing

The task we complete in this part will be slightly similar to 
before when we created a global descriptor table in the sense 
that we are going to create an IDT (Interrupt Descriptor Table). 
And fill it with data (and some other extra helper functions 
both in asm and C) and then it will just work as intended. 
We will start by making a very basic setup for the IDT that just 
handles one interrupt and will then build it further

In my introduction to interrupts i handled the structure of the table perfectly, 
you may have this very surface level understanding and we need to understand 
specifically what we are handling, and what the CPU is handling.

When an event happens, such as dividing by zero (which is the first interrupt we will add), 
the CPU does not know how to respond, right now doing a divide by zero would cause a triple 
fault as it will detect the divide by zero but won't know what to do. 
The CPU's responsibility is to detect an event like this and transfer 
execution to code that is written by us.

Interrupts aren't always done automatically by the CPU though, they can also 
be triggered by us with the `int` instruction, we did this before with the 
BIOS interrupts when printing to the screen. OK, now let's get started.

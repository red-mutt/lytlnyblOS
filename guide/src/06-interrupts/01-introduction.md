# Part VI : Interrupts & IDT

## What are we doing?

The task we complete in this part will be slightly similar to 
before when we created a global descriptor table in the sense 
that we are going to create an IDT (Interrupt Descriptor Table)
and fill it with data (along with some extra helper functions 
both in Assembly and C), and then it will just work as intended. 
We will start by making a basic setup for the IDT that just 
handles one interrupt, and will then build upon it further

In my introduction to interrupts I covered the structure of the table in detail, 
you may only have a surface-level understanding, so we need to understand
specifically what we are responsible for handling and what the CPU handles for us.

When an event happens, such as division by zero (which is the first interrupt we will add), 
the CPU does not know how to respond. Right now, performing a division by zero would cause a triple
fault because it will detect the division by zero but won't know what to do about it.
The CPU's responsibility is to detect an event like this and transfer 
execution to code written by us.

Interrupts aren't always triggered automatically by the CPU, though. They can also
be triggered by us with the `int` instruction. We did this before with the
BIOS interrupts when printing to the screen. Now let's get started.

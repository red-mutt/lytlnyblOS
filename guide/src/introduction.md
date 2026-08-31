# Introduction

## Preface

After looking at the OSDev wiki, I found many of its tutorials confusing and aimless
(although this doesn't really reflect the aim of the OSDev wiki).
Making my own operating system has always been a plan of mine, and
there are also few resources online on how to make operating systems practically. Many books cover the 
complex concepts that go into operating systems with little implementation. Due to this, this guide acts
as a precursor to something like Modern Operating Systems by Andrew S. Tanenbaum. By providing the opposite value, it walks 
you through
a simple implementation of an operating system whilst explaining every detail and providing you with a pretty
nice baseline for what an operating system is.
*"An idiot admires complexity, a genius admires simplicity"* - Some developer ;)

Although my main reason for writing this was to have something for myself to look back on whenever 
I want to go through the building blocks of
making an operating system, which would be really helpful as it's all within my own writing. 
As well as having proof that I know how to make one.
Basically, I'm saying that I'm not looking to revolutionize operating 
system teaching resources (although I do aim to provide a good one).

Surprisingly, there are only a couple of things online that aim at getting beginners started on operating system 
development. 
I aim to demystify the process of writing your own operating system in this book without it taking up too much of 
your time. If there are any problems or things that confuse you in any of my articles, please email me
or message me on any other social media you know I have; 
I am eager to improve both my technical writing and programming.

Although this series of chapters aims to teach beginners, I recommend making sure you know a bit about how any flavour of 
assembly works.
And how C works. Alongside this I would also recommend you know basic concepts of computer science. 
Do you know what a compiler is?
What about a linker? If so then you're OK to move on.
That’s all the required knowledge you would need to get started.
Though assembly is not too necessary, I explain assembly **way** more in depth than C, as it requires 
a lot of familiarity to read naturally.

For most of the chapters in this book, I intend to give you 
information before I give you code. Copying from tutorials is typically not good practice, and 
I do not want to keep anyone trapped in tutorial hell, so for all the chapters 
where it seems plausible (after the first 4), please try making your own implementation and use
the code I provide as inspiration.

## Project Specifications

My operating system is for x86_32 CPU architecture, stored on a virtual floppy disk, and made using C and ASM.
There will also be some other tools used to make the development process a lot easier, such as GDB,
to debug whatever I write, Makefiles to build the project, and QEMU to run the OS without having to 
reboot my system repeatedly.


### Why 32-bit?

I've seen the sentiment in some places on the internet that goes something like "what? Why would you use 32-bit, 
just use 64".
And while this statement would have a lot of merit if we were developing a general purpose operating system,
that does not apply to this project. If we were to use 32 bit the architectural complexity would just 
increase without the main OS features changing.
Long story short, I think that making a 32-bit OS provides a better learning-to-complexity ratio than 64-bit.
It could also be an interesting project to port our OS to a different architecture after we finish,
there are many choices other than just x86_64, such as RISC-V or ARM.
This porting project could teach about what we do for our OS, and what we do differently for architecture.


## Getting Started: What's First?

The first step in making an operating system is to either set up or write a bootloader.
I will be making one; if you wanted to set one up, you could probably set up the kernel you write using GRUB,
I know some people also use Limine.
If a bootloader seems complex, don't feel afraid to use an existing one,
writing a **good** bootloader can be just as large a project as making an operating system.
To write a bootloader, there will only be 3 steps.

Step 1: printing in BIOS that we are booting the OS.
Step 2: reading the hard disk from the right place and loading the kernel into memory (in BIOS).
Step 3: giving access to the kernel by jumping to where we loaded the kernel into memory (still in BIOS).

We also need to remember that we are writing assembly without an operating system.
This means that we can’t use any OS interrupts to print to the screen, read from certain file locations,
or anything else we would have taken for granted when doing our regular programming.

This is a good decomposition of the problem of writing our own bootloader; even now, you could even stop reading.
And Google how to do these steps or even ask ChatGPT (although this is controversial, and I would only recommend it as 
a last resort), 
it’s good to decompose problems for a massive subject like making your own OS,
It can help us learn about the individual aspects rather than getting overwhelmed with information when we Google 
“how to make a bootloader.”
We can google “how to print to BIOS in NASM” and “how to read from a hard disk in BIOS NASM.”

Even printing is complex without an operating system, so don’t get hung up if you take an hour trying to do any aspect of what
we’re doing, especially if you’re just sitting there trying to understand some instructions or code 
(I have done this many times).

In the next chapter, I will cover how to produce the bootloader step by step.

## Acknowledgements

Thanks to Mohammed Q. Hussain for writing "A journey in writing an operating system kernel" 
which I paraphrased from in some places in the initial sections.
Thanks to the OSDev wiki; they provide useful resources on writing drivers
Thanks to the internet for providing me with great resources that helped me explain and understand 
some things along my way of writing this.


## Current State of the Project

Hello, this project is currently in development, and I update it frequently. 
Just for some context, I originally created this guide about 2 years ago and wrote the first 3 entries,
after that, I left the project abandoned until June 2026. I'm now pretty dedicated to this as my main
hobby project and intend to be continually working on it until the guide and source code for the operating
system are up to a high standard


### Writing Quality

I haven't proofread much up to this date, my intention is to finish everything first and make a full draft,
and then I will go back and correct all grammatical and technical mistakes. For this reason, if you are a newcomer,
please be wary using this as a complete guide; most writing about implementing 
features is done, but the whole guide is not complete and the
quality of writing has not had reviewing.


### Code Quality

I am not too well-informed on writing idiomatic C, as of now, I can think of a couple of things in the code base 
that lessen the quality of the code, like representing physical addresses as pointers to my custom-defined types when 
they should be represented as `uintptr_t`. I also struggle to decide between `size_t` and `uint32_t` in some places 
if I remember correctly. There are more inconsistencies and unclean practices.
I find that if I focus too much on writing perfect code, I am a slower developer, making a crude implementation
and then refactoring is an important practice in development, 
and I plan to refactor at the end of the implementing the operating system.
Just like proofreading the document, after I have finished the first draft, I will go back to 
my code and make it as clean as possible. Fret not for the future of this book


# Thank you for reading!

Thank you for reading my guide, it took me around 2.5 months to write it all, and I'm happy you read it.
If you haven't already, please consider giving me a star on [GitHub](https://github.com/red-mutt/lytlnyblOS),
I'd really appreciate it.

I'd like to use this chapter to give you some inspiration on what to do next with the operating system
you've made, using the knowledge you've acquired in this guide you could do some pretty
cool and interesting things, such as:

## Applications
-   **Making games**: You could build games directory on your custom operating system, you could use
    the existing VGA and keyboard interfaces, or you could develop mouse drivers or graphics.
-   **GUI**: You could extend the shell into a GUI of your own design.
-   **Build a text editor**: This is a good task for practicing file manipulation, keyboard input,
    screen output, and memory management. You could even port to handheld hardware and make a device
    for note-taking with your own operating system.
-   **Port exiting software**: Take your favorite C programs and make it run on the OS
    by implementing the functionality that it expects.

## Extension of the OS
-   **Add networking**: Implement an Ethernet driver and eventually TCP/IP
-   **Add more filesystem features**: Nested directories, larger files, permissions,
    timestamps, symlinks, etc...
-   **Add more hardware drivers**

## Technical stuff
-   **Port to another architecture**: You could do ARM, RISC-V, or other
    less familiar architecture.
-   **Move from 32-bit to 64-bit**: Update the boot code, paging, memory management,
    ABI details, pointers, and kernel/user boundaries.
-   **Improve performance**: things like the heap use O(n) time complexity, could you make it O(1)?
    Many other parts you could profile and optimize exist.
-   **Improve security**: OS security is a large rabbit hole to go down, you can try looking for insecurities
    in the existing operating system, and work out methods of patching them.

## Contribute to the guide

You could even contribute to this guide and help improve it, if you're interested, have a look at 
`CONTRIBUTING.md` on the GitHub for `lytlnyblOS`.





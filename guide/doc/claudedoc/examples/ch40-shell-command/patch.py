#!/usr/bin/env python3
"""
Chapter 40 -- adding commands to the shell.

Two new commands, both taking exactly one argument so they fit the
shell's existing rule that every command is two words:

    echo <word>     print the argument
    rev  <word>     print the argument backwards

Adding a command is one else-if branch in execute_command(). The
interesting part is what the rule `if (word_count != 2) return false;`
costs you, which the last edit here demonstrates.
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "common"))
from patchlib import run


def apply(p):
    # ------------------------------------------------------------
    # 1. The commands themselves. They go before the final `else`,
    #    which is the "command not recognised" case.
    # ------------------------------------------------------------
    p.before("user/programs/shell.c",
             "  else {\n    //command not recognised\n    return false;\n  }",
             """  else if (strcmp(words[0], "echo") == 0) {
    printf("%s\\n", words[1]);
  }
  else if (strcmp(words[0], "rev") == 0) {
    size_t n = strlen(words[1]);
    for (size_t i = n; i > 0; i--) {
      putchar(words[1][i - 1]);
    }
    printf("\\n");
  }
  else if (strcmp(words[0], "help") == 0) {
    /* This one only runs when you type "help /" or "help x", because
     * execute_command rejects anything that is not exactly two words.
     * Making "help" work on its own means relaxing that rule, which
     * is the exercise at the end of the chapter. */
    printf("ls mkdir touch rm run cat write echo rev help\\n");
  }""")


sys.exit(run(apply))

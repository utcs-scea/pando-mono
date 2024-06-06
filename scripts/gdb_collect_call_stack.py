# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import gdb
from collections import Counter

# Global function counter
function_counter = Counter()

class CollectFunctions(gdb.Command):
    def __init__(self):
        super(CollectFunctions, self).__init__("collect-functions", gdb.COMMAND_USER)
        gdb.write("Initialized CollectFunctions command.\n")

    def invoke(self, arg, from_tty):
        self.collect_functions()

    def collect_functions(self):
        global function_counter
        frame = gdb.newest_frame()
        functions = []
        while frame:
            function_name = frame.name()
            if function_name:
                functions.append(function_name)
            frame = frame.older()
        function_counter.update(functions)
        gdb.write("Collected functions from current stack frame.\n")

class WriteResults(gdb.Command):
    def __init__(self):
        super(WriteResults, self).__init__("write-results", gdb.COMMAND_USER)
        self.output_file = "call_stack_analysis.txt"
        gdb.write("Initialized WriteResults command.\n")

    def invoke(self, arg, from_tty):
        self.write_results_to_file()

    def write_results_to_file(self):
        global function_counter
        with open(self.output_file, "w") as f:
            f.write("Current function call frequencies:\n")
            for function, count in function_counter.items():
                f.write(f"{function}: {count} times\n")
        gdb.write(f"Results written to {self.output_file}\n")

# Register the commands with GDB
CollectFunctions()
WriteResults()

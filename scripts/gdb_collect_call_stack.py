import gdb
from collections import Counter, defaultdict

# Global counters for functions and lines within those functions
function_counter = Counter()
function_n_values = defaultdict(int)
line_counter = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: {'count': 0, 'sum_n': 0})))

class CollectFunctions(gdb.Command):
    def __init__(self):
        super(CollectFunctions, self).__init__("collect-functions", gdb.COMMAND_USER)
        gdb.write("Initialized CollectFunctions command.\n")

    def invoke(self, arg, from_tty):
        self.collect_functions()

    def collect_functions(self):
        global function_counter, function_n_values, line_counter
        frame = gdb.newest_frame()
        functions = set()  # Use a set to avoid duplicates
        function_details = defaultdict(lambda: defaultdict(list))  # To store line and file details

        while frame:
            if frame.function():
                function_name = frame.function().name
                sal = frame.find_sal()  # Get the symbol address location from frame
                line_number = sal.line
                file_name = sal.symtab.filename if sal.symtab else 'Unknown file'
                if function_name:
                    functions.add(function_name)
                    function_details[function_name][file_name].append(line_number)
            frame = frame.older()

        function_counter.update(functions)

        try:
            n_value = int(gdb.parse_and_eval("n"))
            for function in functions:
                function_n_values[function] += n_value  # Update n value for function
                for file_name, line_numbers in function_details[function].items():
                    for line_number in line_numbers:
                        if line_number not in line_counter[function][file_name]:
                            line_counter[function][file_name][line_number] = {'count': 0, 'sum_n': 0}
                        line_counter[function][file_name][line_number]['count'] += 1
                        line_counter[function][file_name][line_number]['sum_n'] += n_value
            gdb.write(f"Captured value of n: {n_value}\n")
        except gdb.error:
            gdb.write("Error: Could not capture value of 'n'.\n")
        
        gdb.write("Collected functions, file names, line numbers, and values of 'n' from current stack frame.\n")

class WriteResults(gdb.Command):
    def __init__(self):
        super(WriteResults, self).__init__("write-results", gdb.COMMAND_USER)
        self.output_file = "call_stack_analysis.txt"
        gdb.write("Initialized WriteResults command.\n")

    def invoke(self, arg, from_tty):
        self.write_results_to_file()

    def write_results_to_file(self):
        global function_counter, function_n_values, line_counter
        with open(self.output_file, "w") as f:
            f.write("Current function call frequencies, file names, line execution counts, and n values:\n")
            for function, files in line_counter.items():
                f.write(f"{function}: {function_counter[function]} times, n={function_n_values[function]}, average n per call = {function_n_values[function] // function_counter[function] if function_counter[function] > 0 else 'undefined'}\n")
                for file_name, lines in files.items():
                    f.write(f"\tFile {file_name}:\n")
                    for line, data in lines.items():
                        average_n = data['sum_n'] / data['count'] if data['count'] > 0 else 'undefined'
                        f.write(f"\t\tLine {line}: {data['count']} times, total n={data['sum_n']}, average n={average_n}\n")

        gdb.write(f"Results written to {self.output_file}\n")


CollectFunctions()
WriteResults()
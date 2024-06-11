# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import re

def parse_line_details(line):
    match = re.search(r"Line (\d+): (\d+) times, total n=(\d+), average n=([\d.]+)", line.strip())
    if match:
        return {
            'line_number': int(match.group(1)),
            'times': int(match.group(2)),
            'total_n': int(match.group(3)),
            'average_n': float(match.group(4))
        }
    return None

def parse_file_details(lines):
    file_details = []
    current_file = None
    current_lines = []

    for line in lines:
        file_match = re.search(r"^File (.+):", line.strip())
        if file_match:
            if current_file:
                file_details.append({'file_name': current_file, 'lines': current_lines})
            current_file = file_match.group(1)
            current_lines = []
        else:
            line_detail = parse_line_details(line)
            if line_detail:
                current_lines.append(line_detail)

    if current_file:
        file_details.append({'file_name': current_file, 'lines': current_lines})

    return file_details

def parse_function_call(block):
    lines = block.strip().split('\n')
    header = lines[0]
    header_match = re.search(r"^(.+):\s*(\d+)\s*times,\s*n=(\d+),\s*average n per call = ([\d.]+)", header)
    if header_match:
        function_call = {
            'function_name': header_match.group(1).strip(),
            'times': int(header_match.group(2)),
            'total_n': int(header_match.group(3)),
            'average_n_per_call': float(header_match.group(4)),
            'files': parse_file_details(lines[1:])
        }
        return function_call
    else:
        return None  # Return None if the header does not match expected format

def sort_function_calls(function_calls):
    return sorted(function_calls, key=lambda x: (-x['total_n'], -x['times'], -x['average_n_per_call']))

def read_and_sort_file(filename):
    with open(filename, 'r') as file:
        content = file.read()

    # Split into blocks by the pattern that indicates the start of a function block
    blocks = re.split(r'\n(?=\w)', content.strip())
    function_calls = []

    for block in blocks:
        function_call = parse_function_call(block)
        if function_call:  # Only append if the function call is successfully parsed
            function_calls.append(function_call)

    sorted_function_calls = sort_function_calls(function_calls)
    for call in sorted_function_calls:
        found = False
        for file in call['files']:
            if 'pando/include' in file['file_name']:
                found = True  # Skip to the next file if the condition is not met
        #found = True
        if not found :
            continue
        print(f"{call['function_name']}: {call['times']} times, n={call['total_n']}, average n per call = {call['average_n_per_call']}")
        for file in call['files']:
            print(f"\tFile {file['file_name']}:")
            for line_detail in file['lines']:
                print(f"\t\tLine {line_detail['line_number']}: {line_detail['times']} times, total n={line_detail['total_n']}, average n={line_detail['average_n']}")

# Replace 'filename.txt' with the path to your input file
read_and_sort_file('call_stack_analysis.txt')

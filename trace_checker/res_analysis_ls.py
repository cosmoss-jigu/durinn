#!/usr/bin/python3

import argparse
import os
from lib.resanalyzerls import ResAnalyzerLS

def init_src_mapping(allbc):
    curr_dir = os.getcwd()
    allbc_dir = os.path.dirname(allbc)
    os.chdir(allbc_dir)
    os.system('make print_src_map.txt')

    src_mapping = dict()
    with open('print_src_map.txt', 'r') as f:
        lines = f.read().split('\n');
        lines = lines[0:-1]
        for line in lines:
            entry = line.split(',')
            src_num = int(entry[0])
            src_line = entry[1]
            src_type = entry[2]
            if src_type == 'store' or src_type == 'atomic_rmw' \
                    or src_type == 'atomic_cmp' or src_type == 'call':
                src_mapping[src_num] = src_line
    os.chdir(curr_dir)

    return src_mapping

def main():
    parser = argparse.ArgumentParser(description="Ciri Trace Checker")

    parser.add_argument("-o", "--output",
                        required=True,
                        help="")

    parser.add_argument("-allbc", "--allbc",
                        required=True,
                        help="")

    args = parser.parse_args()
    src_mapping = init_src_mapping(args.allbc)

    if os.path.isdir('checker-output-ls3'):
        ResAnalyzerLS('checker-output-ls3', args.output, src_mapping)

    if os.path.isdir('checker-output-ls4'):
        ResAnalyzerLS('checker-output-ls4', args.output, src_mapping)

    if os.path.isdir('checker-output-ls5'):
        ResAnalyzerLS('checker-output-ls5', args.output, src_mapping)

    if os.path.isdir('checker-output-ls6'):
        ResAnalyzerLS('checker-output-ls6', args.output, src_mapping)

if __name__ == "__main__":
    main()

#!/usr/bin/python3

import argparse
from lib.tracesrc import TraceSrc

def main():
    parser = argparse.ArgumentParser(description="Ciri Trace Checker")

    parser.add_argument("-i", "--input",
                        required=True,
                        help="")

    parser.add_argument("-o", "--output",
                        required=True,
                        help="")

    parser.add_argument("-opt", "--opt",
                        required=True,
                        help="")

    parser.add_argument("-tracerdir", "--tracerdir",
                        required=True,
                        help="")

    parser.add_argument("-allbc", "--allbc",
                        required=True,
                        help="")

    args = parser.parse_args()
    trace_src = TraceSrc(args.input, args.output, args.opt, args.tracerdir, args.allbc)

if __name__ == "__main__":
    main()

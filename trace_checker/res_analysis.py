#!/usr/bin/python3

import argparse
import os
from lib.resanalyzer import ResAnalyzer

def main():
    parser = argparse.ArgumentParser(description="Ciri Trace Checker")

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

    if os.path.isdir('checker-output3'):
        res_analyzer = ResAnalyzer('checker-output3', args.output, args.opt, args.tracerdir, args.allbc)

    if os.path.isdir('checker-output4'):
        res_analyzer = ResAnalyzer('checker-output4', args.output, args.opt, args.tracerdir, args.allbc)

    if os.path.isdir('checker-output5'):
        res_analyzer = ResAnalyzer('checker-output5', args.output, args.opt, args.tracerdir, args.allbc)

    if os.path.isdir('checker-output6'):
        res_analyzer = ResAnalyzer('checker-output6', args.output, args.opt, args.tracerdir, args.allbc)

if __name__ == "__main__":
    main()

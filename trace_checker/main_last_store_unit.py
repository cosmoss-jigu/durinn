#!/usr/bin/python3

import argparse
import pickle
from lib.pickle import PICKLE_ARGS, PICKLE_OPFILE, PICKLE_TRACE
from checker.ls_validator import LSValidator

def main():
    parser = argparse.ArgumentParser(description="Ciri Last Store Parallel Unit")

    parser.add_argument("-i", "--i", required=True, help="idx")
    parser.add_argument("-s", "--s", required=True, help="start store id")
    parser.add_argument("-e", "--e", required=True, help="end store id")
    parser.add_argument("-o", "--output", required=True, help="the output dir")

    args = parser.parse_args()

    validator = LSValidator(pickle.load(open(args.output+'/'+PICKLE_TRACE, 'rb')),
                            pickle.load(open(args.output+'/'+PICKLE_OPFILE, 'rb')),
                            args.i,
                            args.s,
                            args.e,
                            pickle.load(open(args.output+'/'+PICKLE_ARGS, 'rb')))
    validator.run()
    validator.clean_up()

if __name__ == "__main__":
    main()

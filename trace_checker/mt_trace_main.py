#!/usr/bin/python3

import argparse
import pickle
from lib.pickle import PICKLE_ARGS, PICKLE_OPFILE
from checker.mt_trace_checker import MTChecker

def main():
    parser = argparse.ArgumentParser(description="Ciri MTTrace")

    parser.add_argument("-i", "--i", required=True, help="pair[0]")
    parser.add_argument("-j", "--j", required=True, help="pair[1]")
    parser.add_argument("-lps", "--lps", required=True, help="lps")
    parser.add_argument("-o", "--output", required=True, help="the output dir")

    args = parser.parse_args()
    pair = (int(args.i), int(args.j))

    lps = args.lps.split(',')
    lps = lps[:-1]
    lps = set([int(lp) for lp in lps])

    mt_checker = MTChecker(pickle.load(open(args.output+'/'+PICKLE_OPFILE, 'rb')),
                           pair,
                           pickle.load(open(args.output+'/'+PICKLE_ARGS, 'rb')),
                           lps)
    mt_checker.validate_race()
    mt_checker.clean_up()

if __name__ == "__main__":
    main()

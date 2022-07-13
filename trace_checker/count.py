#!/usr/bin/python3

import argparse
import os
import pickle
from concurrent.futures import ProcessPoolExecutor, wait
from lib.op_file import OPFile
from lib.trace import Trace
from lib.pickle import PICKLE_ARGS, PICKLE_OPFILE, PICKLE_TRACE
from checker.ls_trace_checker import LSChecker
from lib.fencesrcmap import FenceSrcMap

def main():
    parser = argparse.ArgumentParser(description="Ciri Last Store Trace Checker")

    parser.add_argument("-exe", "--exe",
                        required=True,
                        help="")

    parser.add_argument("-pmsize", "--pmsize",
                        required=True,
                        help="")

    parser.add_argument("-pmlayout", "--pmlayout",
                        required=True,
                        help="")

    parser.add_argument("-op", "--opfile",
                        required=True,
                        help="the op file to process")

    parser.add_argument("-t", "--tracefile",
                        required=True,
                        help="the PM trace file to process")

    parser.add_argument("-opt", "--opt",
                        required=True,
                        help="")

    parser.add_argument("-tracerdir", "--tracerdir",
                        required=True,
                        help="")

    parser.add_argument("-pmaddr", "--pmaddr",
                        required=True,
                        help="")

    parser.add_argument("-name", "--name",
                        required=True,
                        help="")

    parser.add_argument("-allbc", "--allbc",
                        required=True,
                        help="")

    parser.add_argument("-o", "--output",
                        required=True,
                        help="the output dir")

    parser.add_argument("-needdel", "--need-del",
                        required=True,
                        help="need delete in validation or not")

    parser.add_argument("-lockfree", "--lock-free",
                        required=True,
                        help="lock free")

    parser.add_argument("-lockbased", "--lock-based",
                        required=True,
                        help="lock based")

    parser.add_argument("-dstype", "--dstype",
                        required=True,
                        help="ds type")

    args = parser.parse_args()
    os.makedirs(args.output)

    trace = Trace(args.tracefile, True)

    lock_free = int(args.lock_free)
    lock_based = int(args.lock_based)

    trace_checker = LSChecker(trace, lock_free, lock_based, args.output, args.opt, args.allbc, args.tracerdir)

    f = open('durinn.csv', 'w')
    f.write('op,per_op_count,total_count\n')
    total = 0
    idx = 0
    for num in trace_checker.store_list:
        num = num*2
        total += num
        f.write(str(idx) + ',' + str(num) + ',' + str(total) + '\n')
        idx += 1

if __name__ == "__main__":
    main()

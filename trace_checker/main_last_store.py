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

def print_res(trace_checker):
    output = trace_checker.output + '/res.txt'
    f = open(output, 'w')
    f.write('total_stores:' + str(trace_checker.total_stores) + '\n')
    f.write('total_stores_in_op:' + str(trace_checker.total_stores_in_op) + '\n')
    f.write('total_lps:' + str(trace_checker.total_lps) + '\n')
    f.close()

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
    pickle.dump(args, open(args.output+'/'+PICKLE_ARGS, 'wb'))

    op_file = OPFile(args.opfile)
    pickle.dump(op_file, open(args.output+'/'+PICKLE_OPFILE, 'wb'))

    trace = Trace(args.tracefile, True)
    pickle.dump(trace, open(args.output+'/'+PICKLE_TRACE, 'wb'))
    f = open(args.output+'/checker.trace', 'w')
    for op in trace.ops:
        f.write(str(op) + '\n')

    lock_free = int(args.lock_free)
    lock_based = int(args.lock_based)

    trace_checker = LSChecker(trace, lock_free, lock_based, args.output, args.opt, args.allbc, args.tracerdir)

    pool_executor = ProcessPoolExecutor(max_workers=32)
    futures = []
    for job in trace_checker.jobs:
        cmd = [os.environ['DURINN_HOME'] + '/trace_checker/main_last_store_unit.py',
               '-i', str(job[0]),
               '-s', str(job[1]),
               '-e', str(job[2]),
               '-o', args.output]
        cmd = ' '.join(cmd)
        futures.append(pool_executor.submit(os.system, cmd))
    wait(futures)

    print_res(trace_checker)

if __name__ == "__main__":
    main()

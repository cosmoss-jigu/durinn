#!/usr/bin/python3

import argparse
import os
import pickle
from concurrent.futures import ProcessPoolExecutor, wait
from lib.op_file import OPFile
from lib.trace import Trace
from lib.pickle import PICKLE_ARGS, PICKLE_OPFILE
from checker.st_trace_checker import TraceChecker
from lib.fencesrcmap import FenceSrcMap

def main():
    parser = argparse.ArgumentParser(description="Ciri Trace Checker")

    parser.add_argument("-exe", "--exe",
                        required=True,
                        help="")

    parser.add_argument("-traceexe", "--traceexe",
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

    parser.add_argument("-window", "--window",
                        required=True,
                        help="window size for single thread trace")

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

    trace = Trace(args.tracefile)
    fence_src_map = FenceSrcMap(trace, args.opt, args.tracerdir, args.allbc, args.output)

    lock_free = int(args.lock_free)
    lock_based = int(args.lock_based)

    trace_checker = TraceChecker(trace, int(args.window), lock_free, lock_based, args.opt, args.tracerdir, args.allbc)

    #pool_executor = ProcessPoolExecutor(max_workers=os.cpu_count())
    #pool_executor = ProcessPoolExecutor(max_workers=int(os.cpu_count()/2))
    pool_executor = ProcessPoolExecutor(max_workers=32)
    futures = []
    for pair in trace_checker.race_pairs:
        lp_addrs = ''
        for race_pair in trace_checker.race_pairs[pair]:
            for addr in race_pair.race_st.deps:
                lp_addrs += str(addr) + ','
        cmd = [os.environ['DURINN_HOME'] + '/trace_checker/mt_trace_main.py',
               '-i', str(pair[0]),
               '-j', str(pair[1]),
               '-lps', lp_addrs,
               '-o', args.output]
        cmd = ' '.join(cmd)
        futures.append(pool_executor.submit(os.system, cmd))

    wait(futures)

    trace_checker.print_res(args.output)

if __name__ == "__main__":
    main()

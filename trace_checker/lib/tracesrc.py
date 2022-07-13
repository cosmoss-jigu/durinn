import os
import pickle
import shutil
from lib.trace import Trace

SRC_MAP = 'src_map.txt'

class TraceSrc:
    def __init__(self, trace_file, output, opt, tracerdir, allbc):
        self.output = output
        self.trace_file = trace_file

        self.opt = opt
        self.tracerdir = tracerdir
        self.allbc = allbc

        self.res_paths = []

        self.run()

    def run(self):
        os.makedirs(self.output)

        trace = Trace(self.trace_file)

        iids = set()
        for op in trace.ops:
            if op.src_info == 'NA':
                continue
            iids.add(op.src_info)

        cmd = [self.opt,
               '-load ' + self.tracerdir+'/libciri.so',
		       '-mergereturn -lsnum',
               '-dfencesrcmap -fence-ids',
               ','.join(iids),
               '-output',
               self.output + '/' + SRC_MAP,
               '-remove-lsnum -stats ' + self.allbc,
               '-o /dev/null']
        print(' '.join(cmd));
        os.system(' '.join(cmd))

        self.src_map = dict()
        entries = open(self.output+'/'+SRC_MAP).read().split("\n")[:-1]
        for entry in entries:
            strs = entry.split(',')
            inst_id = strs[0]
            src_info = strs[1]
            self.src_map[inst_id] = src_info

        f_trace = open(self.output+'/main.src.trace', 'w')
        for op in trace.ops:
            if op.src_info != 'NA':
                op.src_info = self.src_map[op.src_info]
            f_trace.write(str(op) + '\n')
        f_trace.close()

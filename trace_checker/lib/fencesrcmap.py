from lib.memops import Fence
import os
import pickle

FENCE_SRC_MAP = 'fence_src_map.txt'
FENCE_SRC_MAP_PICKLE = 'fence_src_map.pickle'

class FenceSrcMap:
    def __init__(self, trace, opt, tracerdir, allbc, output):
        self.trace = trace
        self.opt = opt
        self.tracerdir = tracerdir
        self.allbc = allbc
        self.output = output

        self.gen_src_mapping()
        self.gen_src_mapping_pickle()

    def gen_src_mapping(self):
        brk_id_set = set()
        for op in self.trace.ops:
            if isinstance(op, Fence):
                brk_id_set.add(op.src_info)

        cmd = [self.opt,
               '-load ' + self.tracerdir+'/libciri.so',
		       '-mergereturn -lsnum',
               '-dfencesrcmap -fence-ids',
               ','.join(brk_id_set),
               '-output',
               self.output + '/' + FENCE_SRC_MAP,
               '-remove-lsnum -stats ' + self.allbc,
               '-o /dev/null']
        print(' '.join(cmd));
        os.system(' '.join(cmd))

    def gen_src_mapping_pickle(self):
        d = dict()
        entries = open(self.output+'/'+FENCE_SRC_MAP).read().split("\n")[:-1]
        for entry in entries:
            strs = entry.split(',')
            inst_id = strs[0]
            src_info = strs[1]
            d[inst_id] = src_info

        pickle.dump(d, open(self.output+'/'+FENCE_SRC_MAP_PICKLE, 'wb'))

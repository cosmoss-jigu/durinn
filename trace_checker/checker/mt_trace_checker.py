import os
import subprocess
from lib.trace import Trace
from lib.memops import Fence, APIEnd
from lib.filenames import RACE_OP, RACE_OUT, IMG, PREFIX_IMG, PREFIX_OP, PREFIX_OUT
from checker.st_trace_checker_2_ops import TraceChecker
from checker.gdb_checker import GDBChecker
from lib.fencesrcmap import FENCE_SRC_MAP_PICKLE
import pickle

class MTChecker:
    def __init__(self, op_file, pair, args, lps):
        self.op_file = op_file
        self.pair = pair
        self.output = args.output

        self.exe = args.exe
        self.traceexe = args.traceexe
        self.pmsize = args.pmsize
        self.pmlayout = args.pmlayout

        self.opt = args.opt
        self.tracerdir = args.tracerdir
        self.pmaddr = args.pmaddr
        self.name = args.name
        self.allbc = args.allbc

        self.args = args
        self.lps = lps

        self.create_workspace()
        self.gen_prefix()
        self.gen_pmtrace()
        self.check_race()
        os.chdir('..')

    def create_workspace(self):
        os.chdir(self.output)

        self.workspace = str(self.pair[0]) + '-' + str(self.pair[1])
        os.makedirs(self.workspace)
        os.chdir(self.workspace)

    def gen_prefix(self):
        self.op_file.gen_prefix(self.pair, PREFIX_OP)
        cmd = [self.exe,
               IMG,
               self.pmsize,
               self.pmlayout,
               PREFIX_OP,
               PREFIX_OUT,
               '0']
        #print(' '.join(cmd));
        os.system(' '.join(cmd));

        # snapshot the prefix image for gdb checker
        cmd = ['cp', IMG, PREFIX_IMG]
        os.system(' '.join(cmd));

    def gen_pmtrace(self):
        self.op_file.gen_race(self.pair, RACE_OP)
        cmd = [self.traceexe,
               IMG,
               self.pmsize,
               self.pmlayout,
               RACE_OP,
               RACE_OUT,
               '0']
        #print(' '.join(cmd));
        #os.system(' '.join(cmd));
        timeout = False
        my_env = os.environ.copy()
        my_env['PMEM_IS_PMEM_FORCE'] = '0'
        my_env['NO_ALLOC_BR'] = '1'
        try:
            subprocess.run(cmd, timeout=10, env=my_env)
        except subprocess.TimeoutExpired as e:
            timeout = True
            return

    def check_race(self):
        self.races = []

        if not os.path.exists(self.name+'.trace'):
            return

        self.trace = Trace(self.name+'.trace')
        f = open('checker.trace', 'w')
        for op in self.trace.ops:
            f.write(str(op) + '\n')

        if (len(self.trace.ops_api_ranges) != 2):
            return

        trace_checker = TraceChecker(self.trace, 2, self.lps)
        if (0,1) in trace_checker.race_pairs:
            for race in trace_checker.race_pairs[(0,1)]:
                #TODO
                #print(race)
                self.races.append(race)

    def validate_race(self):
        gdb_ops = set()
        for race in self.races:
            gdb_ops.add(race.race_st.op)
        gdb_ops = list(gdb_ops)

        self.total_gdb_tests = len(gdb_ops)
        self.total_gdb_tests_buggy = 0

        for gdb_op in gdb_ops:
            op_id = gdb_op.id
            src_line = gdb_op.src_info
            if src_line == 'NA':
                assert(isinstance(gdb_op, APIEnd))
                # TODO
                src_line = 'api_end'
                hit_count = 1
            else:
                assert(isinstance(gdb_op, Fence))
                d = pickle.load(open(FENCE_SRC_MAP_PICKLE, 'rb'))
                src_line = d[src_line]
                hit_count = self.get_hit_count(gdb_op, d, src_line)

            gdb_checker = GDBChecker(self.pair,
                                     self.args,
                                     op_id,
                                     src_line,
                                     hit_count)
            if gdb_checker.buggy_type == None:
                gdb_checker.clean_up()
            else:
                gdb_checker.clean_up()
                self.total_gdb_tests_buggy += 1

    def clean_up(self):
        if self.total_gdb_tests == 0:
            os.system('rm -r ' + self.workspace)
        else:
            os.system('rm -r ' + self.workspace + '/*.img')
            with open(self.workspace+'/res.txt', 'w') as f:
                f.write('total_gdb_tests:' + str(self.total_gdb_tests) + '\n')
                f.write('total_gdb_tests_buggy:' + str(self.total_gdb_tests_buggy) + '\n')

        os.chdir('..')

    def get_hit_count(self, fence_op, d, src_line):
        hit_count = 0
        op_id = fence_op.id
        ops_api_range = self.trace.ops_api_ranges[0]
        assert(op_id > ops_api_range[0] and op_id < ops_api_range[1])
        for i in range(ops_api_range[0], ops_api_range[1]+1):
            if isinstance(self.trace.ops[i], Fence) and \
                    d[self.trace.ops[i].src_info] == src_line:
                hit_count += 1
            if i == op_id:
                return hit_count
        # never come to here
        assert(False)

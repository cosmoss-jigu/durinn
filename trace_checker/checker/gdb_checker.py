import os
import sys
import subprocess
import pickle
from lib.pickle import PICKLE_GDB_RES
from lib.trace_mt import TraceMT
from lib.binaryfile import BinaryFile
from lib.cache import ReplayCache
from lib.op_file import OPValidator
from lib.filenames import GDB_TEMPLATE, GDB_SCRIPT, CRASH_IMG, VAL_OP, VAL_OUT, ORACLE_OUT
from lib.filenames import RACE_OP, RACE_OUT, IMG, PREFIX_IMG, PREFIX_OP
from lib.op_file_q import OPValidatorQueue

class GDBRes:
    def __init__(self, workspace, trace_file, trace_signature, buggy_type, race_op_type):
        self.workspace = workspace
        self.trace_file = trace_file
        self.trace_signature = trace_signature
        self.buggy_type = buggy_type
        self.race_op_type = race_op_type

class GDBChecker:
    def __init__(self, pair, args, op_id, src_line, hit_count):
        self.pair = pair
        self.op_id = op_id
        self.src_line = src_line
        self.hit_count = hit_count

        self.opt = args.opt
        self.tracerdir = args.tracerdir
        self.pmaddr = args.pmaddr
        self.name = args.name
        self.allbc = args.allbc

        self.output = args.output
        self.traceexe = args.traceexe
        self.pmsize = args.pmsize
        self.pmlayout = args.pmlayout

        self.exe = args.exe

        self.ds_type = int(args.dstype)

        if args.need_del == 'true':
            self.need_del = True
        else:
            self.need_del = False

        self.buggy_type = None

        self.create_workspace()

        self.timeout = False
        self.run_gdb_script()
        # gdb dead lock
        if self.timeout:
            os.chdir('..')
            return
        self.gen_crash_img()
        self.validate()
        self.gen_res()
        os.chdir('..')

    def create_workspace(self):
        #os.chdir(self.output)
        #os.chdir('..')

        self.workspace = str(self.pair[0]) + '-' + str(self.pair[1]) + '-' + str(self.op_id)
        os.makedirs(self.workspace)
        os.chdir(self.workspace)

        self.res_dir = '../' + str(self.pair[0]) + '-' + str(self.pair[1])

        # get prefix image
        cmd = ['cp', self.res_dir+'/'+PREFIX_IMG, IMG]
        os.system(' '.join(cmd));

        # get race op file
        cmd = ['cp', self.res_dir+'/'+RACE_OP, '.']
        os.system(' '.join(cmd));

    def run_gdb_script(self):
        # get gdb file
        with open(GDB_TEMPLATE) as f:
            gdb_script = f.read() \
                          .replace('SRC_LINE', str(self.src_line)) \
                          .replace('HIT_COUNT', str(self.hit_count))
        with open(GDB_SCRIPT, "w") as f:
            f.write(gdb_script)

        cmd = ['gdb',
               '-x',
               GDB_SCRIPT,
               '--args',
               self.traceexe,
               IMG,
               self.pmsize,
               self.pmlayout,
               RACE_OP,
               RACE_OUT,
               '1']
        # os.system(' '.join(cmd));
        my_env = os.environ.copy()
        my_env['PMEM_IS_PMEM_FORCE'] = '0'
        my_env['NO_ALLOC_BR'] = '1'
        my_env['REPLAY_MOD'] = '1'

        #subprocess.run(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, env=my_env)

        # gdb dead lock
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, env=my_env)
        try:
            proc.communicate(timeout=5)
        except subprocess.TimeoutExpired as e:
            proc.kill()
            proc.communicate()
            print('timeout:' + str(self.pair), file=sys.stderr)
            self.timeout = True

    def gen_crash_img(self):
        # load the trace
        self.trace = TraceMT(self.name+'.trace')

        #TODO
        if (len(self.trace.tids) != 3):
            return

        # create the crash img
        cmd = ['cp', self.res_dir+'/'+PREFIX_IMG, CRASH_IMG]
        os.system(' '.join(cmd));

        # create binary file and cache
        self.binary_file = BinaryFile(CRASH_IMG, self.pmaddr)
        self.cache = ReplayCache(self.binary_file)

        # TODO
        ops_thread_idx = self.trace.ops_thread_idx
        assert(len(ops_thread_idx) == 3)
        ops = self.trace.ops
        # TODO
        self.cache.accept_ops(ops[0:ops_thread_idx[1]])
        self.cache.write_back_all_stores()
        self.cache.accept_ops(ops[ops_thread_idx[1]:self.trace.get_roll_back_idx()+1])
        self.cache.accept_ops(ops[ops_thread_idx[2]:])
        self.cache.binary_file.flush()

        cmd = ['cp', CRASH_IMG, CRASH_IMG+'cp']
        os.system(' '.join(cmd));

        f = open('unpersisted_stores.txt', 'w')
        for op in self.cache.get_all_ops():
            f.write(str(op) + '\n')
        f.close()

    def validate(self):
        #TODO
        if (len(self.trace.tids) != 3):
            return

        if self.ds_type == 0:
            self.op_validator = OPValidator(self.res_dir+'/'+PREFIX_OP,
                                        RACE_OP,
                                        RACE_OUT,
                                        self.need_del,
                                        self.src_line == 'api_end')
        elif self.ds_type == 1:
            self.op_validator = OPValidatorQueue(self.res_dir+'/'+PREFIX_OP,
                                        RACE_OP,
                                        RACE_OUT,
                                        self.need_del,
                                        self.src_line == 'api_end')
        else:
            assert(False)
        self.op_validator.gen_model()
        self.op_validator.gen_val_input_and_oracle()
        self.op_validator.exec_prog(self.exe, CRASH_IMG, self.pmsize, self.pmlayout, VAL_OP, VAL_OUT)
        self.buggy_type = self.op_validator.validate(VAL_OUT, ORACLE_OUT)

    def gen_res(self):
        if self.buggy_type == None:
            return
        self.res = GDBRes(self.workspace,
                          self.name+'.trace',
                          self.trace.gen_signature(),
                          self.buggy_type,
                          self.op_validator.race_op_file.race_op_type)
        pickle.dump(self.res, open(PICKLE_GDB_RES, 'wb'))

    def clean_up(self):
        if self.buggy_type == None:
            os.system('rm -r ' + self.workspace)
        else:
            os.system('rm -r ' + self.workspace + '/*.img')
            os.system('rm -r ' + self.workspace + '/*.imgcp')

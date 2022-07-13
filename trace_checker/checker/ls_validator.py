import os
import pickle
import subprocess
from subprocess import Popen, PIPE, TimeoutExpired
from lib.filenames import CRASH_IMG
from lib.utils import get_cacheline_address
from lib.memops import APIBegin, APIEnd, Store
from lib.binaryfile import BinaryFile
from lib.cache import ReplayCache
from lib.witchercache import WitcherCache
from lib.op_file import LSOPValidator
from lib.op_file_q import LSOPValidatorQueue

class LSValidator:
    def __init__(self, trace, op_file, idx, id_start, id_end, args):
        self.trace = trace
        self.op_file = op_file
        self.idx= int(idx)
        self.id_start = int(id_start)
        self.id_end = int(id_end)
        self.output = args.output

        self.exe = args.exe
        self.pmsize = args.pmsize
        self.pmlayout = args.pmlayout

        self.pmaddr = args.pmaddr
        self.name = args.name

        self.ds_type = int(args.dstype)

        self.args = args

        if args.need_del == 'true':
            self.need_del = True
        else:
            self.need_del = False

        self.crashes = pickle.load(open(self.output+'/stores-'+str(idx)+'-'+str(id_start)+'-'+str(id_end), 'rb'))

        self.create_workspace()
        self.init_validator()

        self.res = []

    def create_workspace(self):
        os.chdir(self.output)

        self.workspace = str(self.idx) + '-' + str(self.id_start) + '-' + str(self.id_end)
        os.makedirs(self.workspace)
        os.chdir(self.workspace)

    def init_validator(self):
        if self.ds_type == 0:
            self.op_validator = LSOPValidator(self.op_file,
                                              self.idx,
                                              self.need_del)
        elif self.ds_type == 1:
            self.op_validator = LSOPValidatorQueue(self.op_file,
                                                   self.idx,
                                                   self.need_del)
        else:
            assert(False)
        self.op_validator.gen_model()
        self.op_validator.gen_val_input_and_oracle()

    def run(self):
        with open(CRASH_IMG, "w") as out:
            out.truncate(int(self.pmsize) * 1024 * 1024)
            out.close()

        # create binary file and cache
        self.binary_file = BinaryFile(CRASH_IMG, self.pmaddr)
        #self.cache = ReplayCache(self.binary_file)
        self.cache = WitcherCache(self.binary_file)

        for op in self.trace.ops:
            if op.id > self.id_end:
                return
            if op.id >= self.id_start and op.id <= self.id_end and isinstance(op, Store):
                if op.id in self.crashes:
                    self.gen_crash_and_validate(op)

            is_fence = self.cache.accept_op(op)
            if is_fence:
                self.cache.write_back_all_flushing_stores()
            if isinstance(op, APIBegin) or isinstance(op, APIEnd):
                self.cache.write_back_all_stores()

    def gen_crash_and_validate(self, op):
        # flush the pm.img before copy
        self.cache.binary_file._file_map.flush()

        # case 0
        img_val = str(self.idx) + '-' + str(op.id) + '-0.img'
        cmd = ['cp',
                CRASH_IMG,
                img_val]
        os.system(' '.join(cmd))

        binary_file = BinaryFile(img_val, self.pmaddr)
        for addr in self.cache.cacheline_dict:
            for store in self.cache.cacheline_dict[addr].stores_list:
                binary_file.do_store(store)
        self.validate(img_val, op, 0)

        # case 1
        img_val = str(self.idx) + '-' + str(op.id) + '-1.img'
        cmd = ['cp',
                CRASH_IMG,
                img_val]
        os.system(' '.join(cmd))

        binary_file = BinaryFile(img_val, self.pmaddr)
        for store in self.cache.get_cacheline_from_address(op.address).stores_list:
            binary_file.do_store(store)
        binary_file.do_store(op)
        self.validate(img_val, op, 1)

    def validate(self, crash_state_file, store, crash_info):
        self.op_validator.exec_prog(self.exe,
                                    crash_state_file,
                                    self.pmsize,
                                    self.pmlayout,
                                    crash_state_file+'-op',
                                    crash_state_file+'-out')
        buggy_type = self.op_validator.validate(crash_state_file+'-out',
                                                'oracle')
        if buggy_type:
            res =  'op_idx:' + str(self.idx) + ','
            res += 'store_id:' + str(store.id) + ','
            res += 'store_src_info:' + str(store.src_info) + ','
            res += 'crash_info:' + str(crash_info) + ','
            res += 'buggy_type:' + buggy_type + '\n'
            self.res.append(res)

    def clean_up(self):
        if len(self.res):
            with open('BUG', "w") as f:
                for r in self.res:
                    f.write(r)
        os.chdir('..')
        if not len(self.res):
            os.system('rm -r ' + self.workspace)
        else:
            os.system('rm -r ' + self.workspace+'/*.img')

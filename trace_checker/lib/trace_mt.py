from lib.memops import Store
from lib.memops import Load
from lib.memops import Flush
from lib.memops import Fence
from lib.memops import Lock
from lib.memops import UnLock
from lib.memops import APIBegin
from lib.memops import APIEnd

class TraceMT:
    def __init__(self, arg_trace):
        self.trace = open(arg_trace).read().split("\n")
        self.trace = self.trace[:-1]

        self.tids = []
        self.ops_thread_idx = []

        self.ops = self.extract_operations()

    def extract_operations(self):
        ops = []
        op_curr_index = 0
        op_curr_api_begin_index = -1

        # TODO
        skip_lock_modeling = False

        for entry in self.trace:
            strs = entry.split(",")
            op_type = strs[0]
            tid = None

            if op_type == "LockBegin" or op_type == "UnLockBegin":
                skip_lock_modeling = True
                continue

            if op_type == "LockEnd":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                ops.append(Lock(address, op_curr_index, tid, src_info))

                op_curr_index += 1
                skip_lock_modeling = False
                continue

            if op_type == "UnLockEnd":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                ops.append(UnLock(address, op_curr_index, tid, src_info))

                op_curr_index += 1
                skip_lock_modeling = False
                continue

            if skip_lock_modeling:
                continue

            if op_type == "Store":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                size = int(strs[3], 10)
                value = strs[4].split(" ")
                src_info = strs[5]
                is_cas = int(strs[6])
                ops.append(Store(address, size, value, op_curr_index, tid, src_info, is_cas))

            elif op_type == "Load":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                size = int(strs[3], 10)
                src_info = strs[4]
                ops.append(Load(address, size, op_curr_index, tid, src_info))

            elif op_type == "Flush":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                ops.append(Flush(address, op_curr_index, tid, src_info))

            elif op_type == "Fence":
                tid = int(strs[1], 10)
                src_info = strs[2]
                ops.append(Fence(op_curr_index, tid, src_info))

            elif op_type == "Lock":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                ops.append(Lock(address, op_curr_index, tid, src_info))

            elif op_type == "UnLock":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                ops.append(UnLock(address, op_curr_index, tid, src_info))

            elif op_type == "APIBegin":
                tid = int(strs[1], 10)
                src_info = strs[2]
                ops.append(APIBegin(op_curr_index, tid, src_info))

            elif op_type == "APIEnd":
                tid = int(strs[1], 10)
                src_info = strs[2]
                ops.append(APIEnd(op_curr_index, tid, src_info))

            elif op_type == "Alloc":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                size = int(strs[3], 10)
                src_info = strs[4]
                op_curr_index -= 1

            elif op_type == "Br":
                tid = int(strs[1], 10)
                src_info = strs[2]
                op_curr_index -= 1

            else:
                print(op_type)
                assert(False);

            assert(tid != None)
            if tid not in self.tids:
                self.tids.append(tid)
                self.ops_thread_idx.append(op_curr_index)

            op_curr_index += 1

        # TODO
        # assert(len(self.tids) == 3)
        return ops

    def get_roll_back_idx(self):
        start = self.ops_thread_idx[2] - 1
        end = self.ops_thread_idx[1]
        for idx in range(start, end, -1):
            op = self.ops[idx]
            if isinstance(op, Store):
                return idx
            elif isinstance(op, Flush):
                continue
            elif isinstance(op, Fence):
                return idx

    def gen_signature(self):
        sig = ''
        t1_sig = set()
        t2_sig = set()

        for op in self.ops[self.ops_thread_idx[1]:self.ops_thread_idx[2]]:
            if isinstance(op, Load) or isinstance(op, Store) or \
                    isinstance(op, Flush) or isinstance(op, Fence):
                t1_sig.add(op.src_info)

        for op in self.ops[self.ops_thread_idx[2]:]:
            if isinstance(op, Load) or isinstance(op, Store) or \
                    isinstance(op, Flush) or isinstance(op, Fence):
                t2_sig.add(op.src_info)

        sig += 't1:' + ','.join(sorted(t1_sig))
        sig += ',t2:' + ','.join(sorted(t2_sig))
        return sig

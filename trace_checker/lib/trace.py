from lib.memops import Store
from lib.memops import Load
from lib.memops import Flush
from lib.memops import Fence
from lib.memops import Lock
from lib.memops import UnLock
from lib.memops import APIBegin
from lib.memops import APIEnd
from lib.utils import ATOMIC_WRITE_BYTES

class Trace:
    def __init__(self, arg_trace, only_persist=False):
        self.trace = open(arg_trace).read().split("\n")
        self.trace = self.trace[:-1]

        self.ops_api_ranges = []

        self.load_addrs = set()
        self.load_src_to_addrs = dict()
        self.allocs = dict()
        self.br_srcs = set()

        self.ops = self.extract_operations(only_persist)

    # split a store entry in ATOMIC_WRITE_BYTES size
    def get_stores(self, params, atomic_write_op_curr_index):
        tid = int(params[1], 10)
        address = int(params[2], 16)
        size = int(params[3], 10)
        value = params[4].split(" ")
        src_info = params[5]
        is_cas = int(params[6])

        atomic_store_list = []

        address_atomic_write_low = int(address/ATOMIC_WRITE_BYTES) * \
                                   ATOMIC_WRITE_BYTES
        address_atomic_write_high = address_atomic_write_low + \
                                    ATOMIC_WRITE_BYTES
        if address > address_atomic_write_low:
            size_first_chunk = \
                              min(size, address_atomic_write_high - address)
            atomic_store = Store(address,
                                 size_first_chunk,
                                 value[0:size_first_chunk],
                                 atomic_write_op_curr_index,
                                 tid,
                                 src_info,
                                 is_cas)
            atomic_write_op_curr_index = atomic_write_op_curr_index + 1
            atomic_store_list.append(atomic_store)
            curr_offset = address_atomic_write_high - address
        else:
            curr_offset = 0
        while curr_offset + ATOMIC_WRITE_BYTES <= size:
            atomic_store = Store(address+curr_offset,
                                 ATOMIC_WRITE_BYTES,
                                 value[curr_offset:curr_offset+ATOMIC_WRITE_BYTES],
                                 atomic_write_op_curr_index,
                                 tid,
                                 src_info,
                                 is_cas)
            atomic_write_op_curr_index = atomic_write_op_curr_index + 1
            atomic_store_list.append(atomic_store)
            curr_offset += ATOMIC_WRITE_BYTES

        if size - curr_offset > 0:
            atomic_store = Store(address+curr_offset,
                                 size-curr_offset,
                                 value[curr_offset:],
                                 atomic_write_op_curr_index,
                                 tid,
                                 src_info,
                                 is_cas)
            atomic_write_op_curr_index = atomic_write_op_curr_index + 1
            atomic_store_list.append(atomic_store)

        return atomic_store_list

    def extract_operations(self, only_persist):
        ops = []
        op_curr_index = 0
        op_curr_api_begin_index = -1
        curr_idx = -1

        # TODO
        skip_lock_modeling = False

        for entry in self.trace:
            strs = entry.split(",")
            op_type = strs[0]

            if op_type == "LockBegin" or op_type == "UnLockBegin":
                skip_lock_modeling = True
                continue

            if op_type == "LockEnd":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                if not only_persist:
                    ops.append(Lock(address, op_curr_index, tid, src_info))
                else:
                    op_curr_index -= 1

                op_curr_index += 1
                skip_lock_modeling = False
                continue

            if op_type == "UnLockEnd":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                if not only_persist:
                    ops.append(UnLock(address, op_curr_index, tid, src_info))
                else:
                    op_curr_index -= 1

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
                if not only_persist:
                    ops.append(Store(address, size, value, op_curr_index, tid, src_info, is_cas))
                else:
                    atomic_store_list = self.get_stores(strs, op_curr_index)
                    ops += atomic_store_list
                    op_curr_index += len(atomic_store_list) - 1

            elif op_type == "Load":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                size = int(strs[3], 10)
                src_info = strs[4]
                if not only_persist:
                    ops.append(Load(address, size, op_curr_index, tid, src_info))
                else:
                    op_curr_index -= 1

                if op_curr_api_begin_index != -1:
                    self.load_addrs.add(address)
                    if src_info not in self.load_src_to_addrs:
                        self.load_src_to_addrs[src_info] = set()
                    self.load_src_to_addrs[src_info].add(address)

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
                if not only_persist:
                    ops.append(Lock(address, op_curr_index, tid, src_info))
                else:
                    op_curr_index -= 1

            elif op_type == "UnLock":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                src_info = strs[3]
                if not only_persist:
                    ops.append(UnLock(address, op_curr_index, tid, src_info))
                else:
                    op_curr_index -= 1


            elif op_type == "APIBegin":
                tid = int(strs[1], 10)
                src_info = strs[2]
                ops.append(APIBegin(op_curr_index, tid, src_info))

                if op_curr_api_begin_index != -1:
                    self.ops_api_ranges.append([op_curr_api_begin_index,
                                                op_curr_index])
                op_curr_api_begin_index = op_curr_index
                curr_idx += 1

            elif op_type == "APIEnd":
                tid = int(strs[1], 10)
                src_info = strs[2]
                ops.append(APIEnd(op_curr_index, tid, src_info))

                assert(op_curr_api_begin_index != -1)
                self.ops_api_ranges.append([op_curr_api_begin_index,
                                           op_curr_index])
                op_curr_api_begin_index = -1

            elif op_type == "Alloc":
                tid = int(strs[1], 10)
                address = int(strs[2], 16)
                size = int(strs[3], 10)
                src_info = strs[4]
                if op_curr_api_begin_index != -1:
                    if curr_idx not in self.allocs:
                        self.allocs[curr_idx] = []
                    self.allocs[curr_idx].append(Load(address, size, 0, tid, src_info))
                op_curr_index -= 1

            elif op_type == "Br":
                if op_curr_api_begin_index != -1:
                    tid = int(strs[1], 10)
                    src_info = strs[2]
                    self.br_srcs.add(src_info)
                op_curr_index -= 1

            else:
                print(op_type)
                assert(False);

            op_curr_index += 1

        self.trace = None
        return ops

    def gen_signature_ls(self, op_idx):
        store_src_infos = set()
        op_range = self.ops_api_ranges[op_idx]
        for op in self.ops[op_range[0]:op_range[1]+1]:
            if isinstance(op, Store):
                store_src_infos.add(op.src_info)
        sig = ''
        for s in store_src_infos:
            sig += s + ','
        return sig

    def gen_signature(self):
        assert(len(self.ops_api_ranges) == 2)
        sig = ''
        t1 = None
        t1_sig = set()
        t2 = None
        t2_sig = set()
        for op in self.ops[self.ops_api_ranges[0][0]:self.ops_api_ranges[1][1]]:
            if isinstance(op, Load) or isinstance(op, Store) or \
                    isinstance(op, Flush) or isinstance(op, Fence):
                if t1 == None and t2 == None:
                    t1 = op.tid
                if t1 != None and t2 == None and op.tid != t1:
                    t2 = op.tid

                if t1 != None and t2 == None:
                    t1_sig.add(op.src_info)
                elif t1!= None and t2 != None:
                    t2_sig.add(op.src_info)
                else:
                    # never come to here
                    assert(True)
        sig += 't1:' + ','.join(sorted(t1_sig))
        sig += ',t2:' + ','.join(sorted(t2_sig))
        return sig

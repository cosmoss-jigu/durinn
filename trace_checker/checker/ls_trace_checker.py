import pickle
import os
from lib.memops import Store
from lib.memops import Load
from lib.memops import Flush
from lib.memops import Fence
from lib.memops import Lock
from lib.memops import UnLock
from lib.memops import APIBegin
from lib.memops import APIEnd
from lib.cache import Cache
from lib.utils import get_cacheline_address, range_cmp

Threshold = 10000

class LSChecker:
    def __init__(self, trace, lock_free, lock_based, output, opt, allbc, tracerdir):
        self.trace = trace
        self.lock_free = lock_free
        self.lock_based = lock_based
        self.output = output
        self.opt = opt
        self.allbc = allbc
        self.tracerdir = tracerdir

        self.init_res()

        self.jobs = []
        self.lps = set()
        self.init_jobs()

    def init_res(self):
        self.total_stores = len([op for op in self.trace.ops if isinstance(op, Store)])
        self.total_stores_in_op = 0
        self.total_lps = 0
        self.store_list = []

    def init_jobs(self):
        if self.lock_based == 4 or self.lock_based == 6:
            f = open('brs.txt', 'w')
            for br_src in self.trace.br_srcs:
               f.write(br_src + '\n');
            f.close()

            cmd = [self.opt,
                   '-load ' + self.tracerdir+'/libciri.so',
                   '-mergereturn -lsnum',
                   '-dprintlpsfrombrs',
                   '-remove-lsnum ' + self.allbc]
            os.system(' '.join(cmd))

            lp_srcs = open('lps_from_brs.txt').read().split('\n')
            lp_srcs = lp_srcs[0:-1]
            self.lp_addrs = set()
            for lp_src in lp_srcs:
                if lp_src in self.trace.load_src_to_addrs:
                    for lp_addr in self.trace.load_src_to_addrs[lp_src]:
                        self.lp_addrs.add(lp_addr)
            #for lp_addr in self.lp_addrs:
            #    print(hex(lp_addr))

        for idx, api_range in enumerate(self.trace.ops_api_ranges):
            trace_per_api = self.trace.ops[api_range[0] : api_range[1]+1]
            self.init_jobs_per_api(trace_per_api, idx)

    def init_jobs_per_api(self, trace_per_api, idx):
        stores = [op for op in trace_per_api if isinstance(op, Store)]
        self.total_stores_in_op += len(stores)

        if self.lock_free:
            stores = [store for store in stores if store.is_cas]

        # all stores
        if self.lock_based == 0:
            pass
        # stores after fence
        elif self.lock_based == 1:
            stores = self.stores_after_fence(trace_per_api)
        # stores used by read
        elif self.lock_based == 2:
            stores = self.stores_used_by_loads(stores)
        # stores not in new(malloc)
        elif self.lock_based == 3:
            stores = self.stores_not_in_new(stores, idx)
        # stores used by reads before if
        elif self.lock_based == 4:
            stores = self.stores_used_by_loads_before_br(stores)
        elif self.lock_based == 5:
            stores = self.stores_match_lps(stores)
        elif self.lock_based == 6:
            stores = self.stores_not_in_new(stores, idx)
            stores = self.stores_used_by_loads_before_br(stores)

        self.store_list.append(len(stores))
        if len(stores) == 0:
            return

        self.total_lps += len(stores)

        for i in range(1, int(len(stores) / Threshold)):
            id_start = stores[(i-1)*Threshold].id
            id_end = stores[i*Threshold-1].id
            self.jobs.append([idx, id_start, id_end])
            stores_unit = stores[(i-1)*Threshold : i*Threshold]
            stores_unit = [store.id for store in stores_unit]
            sotres_unit = set(stores_unit)
            self.write_to_file(stores_unit, idx, id_start, id_end)

        i = int(len(stores) / Threshold)
        id_start = stores[i*Threshold].id
        id_end = stores[-1].id
        self.jobs.append([idx, id_start, id_end])
        stores_unit = stores[i*Threshold:]
        stores_unit = [store.id for store in stores_unit]
        sotres_unit = set(stores_unit)
        self.write_to_file(stores_unit, idx, id_start, id_end)

    def stores_after_fence(self, trace_per_api):
        ret = []
        last_fence = None
        for op in trace_per_api:
            if last_fence and isinstance(op, Store):
                ret.append(op)
                last_fence = None
                continue
            if isinstance(op, Fence):
                last_fence = op
                continue
        return ret

    def stores_used_by_loads(self, stores):
        return [store for store in stores if store.address in self.trace.load_addrs]

    def stores_not_in_new(self, stores, idx):
        allocs = self.trace.allocs
        if idx not in allocs:
            return stores
        else:
            ret = []
            for store in stores:
                not_in_new = True
                for alloc in allocs[idx]:
                    if range_cmp(store, alloc) == 0:
                        not_in_new = False
                        break
                if not_in_new:
                    ret.append(store)
            return ret

    def stores_used_by_loads_before_br(self, stores):
        return [store for store in stores if store.address in self.lp_addrs]

    def stores_match_lps(self, stores):
        if len(self.lps) == 0:
            lps = open('lp.txt').read().split('\n')
            for lp in lps:
                if len(lp) == 0:
                    continue
                self.lps.add(lp)
        return [store for store in stores if store.src_info in self.lps]


    def write_to_file(self, stores_unit, idx, id_start, id_end):
        file_name = self.output + "/stores-" + str(idx) + "-" + str(id_start) + "-" + str(id_end)
        pickle.dump(stores_unit, open(file_name, 'wb'))

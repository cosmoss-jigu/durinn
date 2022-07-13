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
from lib.race import RaceOp
from lib.race import RacePair
from lib.race import racing
from lib.utils import range_cmp

class TraceChecker:
    def __init__(self, trace, window, lock_free, lock_based, opt, tracerdir, allbc):
        self.trace = trace
        self.window = window
        self.lock_free = lock_free
        self.lock_based = lock_based
        self.opt = opt
        self.tracerdir = tracerdir
        self.allbc = allbc

        self.lps = set()

        self.init_res()

        self.race_st_list = []
        self.race_ld_list = []
        self.init_race_op_list()

        self.race_pairs = dict()

        self.init_race_pairs()

        #self.print_race_pairs()

    def init_res(self):
        self.total_stores = len([op for op in self.trace.ops if isinstance(op, Store)])
        self.total_stores_in_op = 0
        self.total_lps = 0
        self.total_races_st = 0

    def init_race_op_list(self):
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

        for idx, api_range in enumerate(self.trace.ops_api_ranges):
            trace_per_api = self.trace.ops[api_range[0] : api_range[1]+1]
            self.init_race_op_list_per_api(trace_per_api, idx)

    def init_race_op_list_per_api(self, trace_per_api, idx):
        lp_stores = [op for op in trace_per_api if isinstance(op, Store)]
        self.total_stores_in_op += len(lp_stores)

        if self.lock_free:
            lp_stores = [store for store in lp_stores if store.is_cas]

        # all stores
        if self.lock_based == 0:
            pass
        # stores after fence
        elif self.lock_based == 1:
            #stores = self.stores_after_fence(trace_per_api)
            pass
        # stores used by read
        elif self.lock_based == 2:
            #stores = self.stores_used_by_loads(stores)
            pass
        # stores not in new(malloc)
        elif self.lock_based == 3:
            lp_stores = self.stores_not_in_new(lp_stores, idx)
        # stores used by reads before if
        elif self.lock_based == 4:
            lp_stores = self.stores_used_by_loads_before_br(lp_stores)
        elif self.lock_based == 5:
            lp_stores = self.stores_match_lps(lp_stores)
        elif self.lock_based == 6:
            lp_stores = self.stores_not_in_new(lp_stores, idx)
            lp_stores = self.stores_used_by_loads_before_br(lp_stores)

        self.total_lps += len(lp_stores)

        race_st_list = []
        race_ld_list = []

        locks = []
        locks_all = []

        cache_st = Cache()
        cache_ld = Cache()

        no_stores = True

        for op in trace_per_api:
            if isinstance(op, Store):
                if not cache_ld.is_empty():
                    loads = cache_ld.get_all_ops()
                    race_ld = RaceOp(op, loads, locks_all)
                    race_ld_list.append(race_ld)

                cache_st.accept_op(op)
                cache_ld.clear()
                if no_stores:
                    no_stores = False

            elif isinstance(op, Load):
                cache_ld.accept_op(op)

            elif isinstance(op, Flush):
                cache_st.accept_flush(op)
                cache_ld.accept_flush(op)

            elif isinstance(op, Fence):
                stores = cache_st.get_flushing_ops()
                stores = [store for store in stores if store in lp_stores]
                if len(stores) > 0:
                    race_st = RaceOp(op, stores, locks)
                    race_st_list.append(race_st)

                cache_st.accept_fence()
                cache_ld.accept_fence()

            elif isinstance(op, Lock):
                locks.append(op)
                locks_all.append(op)

            elif isinstance(op, UnLock):
                assert(len(locks) > 0)
                for i in reversed(range(len(locks))):
                    lock = locks[i]
                    if range_cmp(lock, op) == 0:
                        locks.remove(lock)
                        break
                assert(True)

            elif isinstance(op, APIBegin):
                continue;

            elif isinstance(op, APIEnd):
                #assert(len(locks) == 0)
                if len(locks) == 0:
                    if not cache_st.is_empty():
                        stores = cache_st.get_all_ops()
                        stores = [store for store in stores if store in lp_stores]
                        if len(stores) > 0:
                            race_st = RaceOp(op, stores, locks)
                            race_st_list.append(race_st)
                    if no_stores:
                        loads = cache_ld.get_all_ops()
                        race_ld = RaceOp(op, loads, locks_all)
                        race_ld_list.append(race_ld)

            else:
                print(op)
                assert(False)

        self.race_st_list.append(race_st_list)
        self.race_ld_list.append(race_ld_list)

    def init_race_pairs(self):
        num_apis = len(self.trace.ops_api_ranges)
        assert(len(self.race_st_list) == num_apis)
        assert(len(self.race_ld_list) == num_apis)

        for i in range(num_apis):
            right = min(num_apis, i+self.window)
            for j in range(i+1, right):
                self.init_race_pairs_per_2_apis(i, j)

    def init_race_pairs_per_2_apis(self, i, j):
        key = (i, j)
        pairs = []

        for race_st in self.race_st_list[i]:
            for race_ld in self.race_ld_list[j]:
                if racing(race_st, race_ld):
                    pairs.append(RacePair(race_st, race_ld))

        if len(pairs) > 0:
            self.race_pairs[key] = pairs
            self.total_races_st += len(pairs)

        key = (j, i)
        pairs = []

        for race_ld in self.race_ld_list[i]:
            for race_st in self.race_st_list[j]:
                if racing(race_st, race_ld):
                    pairs.append(RacePair(race_st, race_ld))

        if len(pairs) > 0:
            self.race_pairs[key] = pairs
            self.total_races_st += len(pairs)

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

    def print_race_pairs(self):
        f = open('race_pairs.txt', 'w')
        for key in self.race_pairs:
            f.write(str(key) + '\n')
            for pair in self.race_pairs[key]:
                f.write('\t' + str(pair) + '\n')

    def print_res(self, workspace):
        with open(workspace+'/res.txt', 'w') as f:
            f.write('total_stores:' + str(self.total_stores) + '\n')
            f.write('total_stores_in_op:' + str(self.total_stores_in_op) + '\n')
            f.write('total_lps:' + str(self.total_lps) + '\n')
            f.write('total_races_st:' + str(self.total_races_st) + '\n')

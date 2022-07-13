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
    def __init__(self, trace, window, lps):
        self.trace = trace
        self.window = window
        self.lps = lps

        self.race_st_list = []
        self.race_ld_list = []
        self.init_race_op_list()

        self.race_pairs = dict()
        self.init_race_pairs()

        #self.print_race_pairs()

    def init_race_op_list(self):
        for api_range in self.trace.ops_api_ranges:
            trace_per_api = self.trace.ops[api_range[0] : api_range[1]+1]
            self.init_race_op_list_per_api(trace_per_api)

    def init_race_op_list_per_api(self, trace_per_api):
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
                stores = [store for store in stores if store.address in self.lps]
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
                assert(len(locks) == 0)
                if not cache_st.is_empty():
                    stores = cache_st.get_all_ops()
                    stores = [store for store in stores if store.address in self.lps]
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

        key = (j, i)
        pairs = []

        for race_ld in self.race_ld_list[i]:
            for race_st in self.race_st_list[j]:
                if racing(race_st, race_ld):
                    pairs.append(RacePair(race_st, race_ld))

        if len(pairs) > 0:
            self.race_pairs[key] = pairs

    def print_race_pairs(self):
        f = open('race_pairs.txt', 'w')
        for key in self.race_pairs:
            f.write(str(key) + '\n')
            for pair in self.race_pairs[key]:
                f.write('\t' + str(pair) + '\n')

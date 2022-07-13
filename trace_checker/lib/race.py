class RaceOp:
    def __init__(self, op, deps, lock_set):
        self.op = op;
        self.deps = set([dep.address for dep in deps])
        self.lock_set = set([lock.address for lock in lock_set])

    def __str__(self):
        return "op:" + str(self.op) + \
               "deps:" + str(self.deps) + \
               "locks:" + str(self.lock_set)

class RacePair:
    def __init__(self, race_st, race_ld):
        self.race_st = race_st
        self.race_ld = race_ld

    def __str__(self):
        return "RacePair:" + '\n' + \
                '\t' + str(self.race_st) + '\n' + \
                '\t' + str(self.race_ld)

def racing(race_st, race_ld):
    return len(race_st.deps.intersection(race_ld.deps)) > 0 and \
            len(race_st.lock_set.intersection(race_ld.lock_set)) == 0

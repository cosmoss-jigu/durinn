from lib.memops import Store, Flush, Fence

class Cache:
    def __init__(self):
        self.ops = []

    def accept_op(self, op):
        self.ops.append(op)

    def accept_flush(self, flush):
        for op in self.ops:
            if flush.is_in_flush(op):
                op.flushing = 1

    def accept_fence(self):
        self.ops = list(filter(lambda op: op.flushing == 0, self.ops))

    def clear(self):
        self.ops.clear()

    def is_empty(self):
        return len(self.ops) == 0

    def get_all_ops(self):
        return self.ops;

    def get_flushing_ops(self):
        return list(filter(lambda op: op.flushing == 1, self.ops))

class ReplayCache(Cache):
    def __init__(self, binary_file):
        self.ops = []
        self.binary_file = binary_file

    def accept_ops(self, ops):
        for op in ops:
            self.accept_op(op)

    def accept_op(self, op):
        if isinstance(op, Store):
            self.accept_store(op)
        elif isinstance(op, Flush):
            self.accept_flush(op)
        elif isinstance(op, Fence):
            self.accept_fence()
        else:
            # never come to here
            assert(True)

    def accept_store(self, store):
        self.ops.append(store)

    def accept_fence(self):
        for op in self.get_flushing_ops():
            self.binary_file.do_store(op)
        self.ops = list(filter(lambda op: op.flushing == 0, self.ops))

    def write_back_all_stores(self):
        for op in self.ops:
            self.binary_file.do_store(op)
        self.ops = []

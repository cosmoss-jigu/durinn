from lib.utils import memory_map

class BinaryFile:
    def __init__(self, file_name, map_base):
        self.file_name = file_name
        self.map_base = int(map_base, 16)

        self._file_map = memory_map(file_name)

    def __str__(self):
        return self.file_name

    # Write using a store op
    #def do_store(self, store_op):
    #    base_off = store_op.get_base_address() - self.map_base
    #    max_off = store_op.get_max_address() - self.map_base
    #    curr_base = base_off
    #    curr_value_base = 0
    #    step = 64
    #    while curr_base <= max_off:
    #        if curr_base + step < max_off:
    #            curr_max = curr_base + step
    #            curr_value_max = curr_value_base + step
    #        else:
    #            curr_max = max_off
    #            curr_value_max = curr_value_base + max_off - base_off
    #        self._file_map[curr_base:curr_max] = store_op.value_bytes[curr_value_base:curr_value_max]
    #        #self._file_map.flush(curr_base & ~4095, 4096)

    #        curr_base += step
    #        curr_value_base += step

    #    # only flush before crash
    #    # self._file_map.flush(base_off & ~4095, 4096)

    def do_store(self, store_op):
        base_off = store_op.get_base_address() - self.map_base
        max_off = store_op.get_max_address() - self.map_base
        self._file_map[base_off:max_off] = store_op.value_bytes
        # only flush before crash
        # self._file_map.flush(base_off & ~4095, 4096)

    # write using base_off, max_off and val, this is used by PMDK Trace Handler
    def do_store_direct(self, base_off, max_off, val):
        self._file_map[base_off:max_off] = val
        # only flush before crash
        # self._file_map.flush(base_off & ~4095, 4096)

    # only flush before crash
    def flush(self):
        self._file_map.flush()
        self._file_map.close()

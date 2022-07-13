from lib.utils import Rangeable
from lib.utils import range_cmp
from lib.utils import CACHELINE_BYTES
from lib.utils import get_cacheline_address
from sys import byteorder

class BaseOperation:
    def __init__(self, op_id, tid, src_info):
        self.id = op_id
        self.tid = tid
        self.src_info = src_info

    def __repr__(self):
        return "id:" + str(self.id) + ",tid:" + str(self.tid) + ",src_info:" + str(self.src_info)

    def __str__(self):
        return "id:" + str(self.id) + ",tid:" + str(self.tid) + ",src_info:" + str(self.src_info)

# API Begin Operation
class APIBegin(BaseOperation):
    def __repr__(self):
        return "APIBegin:" + BaseOperation.__repr__(self)

    def __str__(self):
        return "APIBegin:" + BaseOperation.__str__(self)

# API End Operation
class APIEnd(BaseOperation):
    def __repr__(self):
        return "APIEnd:" + BaseOperation.__repr__(self)

    def __str__(self):
        return "APIEnd:" + BaseOperation.__str__(self)

# Store Operation
class Store(BaseOperation, Rangeable):
    def __init__(self, address, size, value, op_id, tid, src_info, is_cas):
        self.address = address
        self.size = size
        self.flushing = 0

        # Initialize the byte representation of value for writing to a file
        self.value = value
        value_bytes = ""
        for byte in value:
            value_bytes = value_bytes + byte
        self.value_bytes = bytes(bytearray.fromhex(value_bytes))

        self.is_cas = is_cas

        BaseOperation.__init__(self, op_id, tid, src_info)

    def __repr__(self):
        return "Store:" + \
               "addr:" + hex(self.address) + \
               ",size:" + str(self.size) + \
               ",value:" + str(self.value) + \
               "," + BaseOperation.__str__(self) + \
               ",is_cas:" + str(self.is_cas)

    def __str__(self):
        return "Store:" + \
               "addr:" + hex(self.address) + \
               ",size:" + str(self.size) + \
               ",value:" + str(self.value) + \
               "," + BaseOperation.__str__(self) + \
               ",is_cas:" + str(self.is_cas)

    # Rangeable implementation
    def get_base_address(self):
        return self.address

    # Rangeable implementation
    def get_max_address(self):
        return self.address + self.size

    # mark it and to be flushed when fence
    def accept_flush(self):
        assert(self.is_clear() or self.is_flushing())
        self.flushing = 1

    # already flushed and written back
    def accept_fence(self):
        assert(self.is_clear() or self.is_flushing())
        self.flushing = 2

    # check it is clear
    def is_clear(self):
        return self.flushing == 0

    # check it is to be flushed when fence
    def is_flushing(self):
        return self.flushing == 1

    # check it is fenced
    def is_fenced(self):
        return self.flushing == 2

# Load Operation
class Load(BaseOperation, Rangeable):
    def __init__(self, address, size, op_id, tid, src_info):
        self.address = address
        self.size = size
        self.flushing = 0
        BaseOperation.__init__(self, op_id, tid, src_info)

    def __repr__(self):
        return "Load:" + \
               "addr:" + hex(self.address) + \
               ",size:" + str(self.size) + \
               "," + BaseOperation.__str__(self)

    def __str__(self):
        return "Load:" + \
               "addr:" + hex(self.address) + \
               ",size:" + str(self.size) + \
               "," + BaseOperation.__str__(self)

    # Rangeable implementation
    def get_base_address(self):
        return self.address

    # Rangeable implementation
    def get_max_address(self):
        return self.address + self.size

# FlushBase interface
class FlushBase(BaseOperation, Rangeable):
    def is_in_flush(self, store_op):
        raise NotImplementedError

# Flush Operation
class Flush(FlushBase):
    def __init__(self, address_not_aligned, op_id, tid, src_info):
        self.address_not_aligned = address_not_aligned
        self.address = get_cacheline_address(address_not_aligned)
        self.size = CACHELINE_BYTES
        BaseOperation.__init__(self, op_id, tid, src_info)

    def __repr__(self):
        return "Flush:" + \
               "addr:" + hex(self.address) + \
               ",size:" + str(self.size) + \
               "," + BaseOperation.__str__(self)

    def __str__(self):
        return "Flush:" + \
               "addr:" + hex(self.address) + \
               ",size:" + str(self.size) + \
               "," + BaseOperation.__str__(self)

    # FlushBase implementation
    def is_in_flush(self, store_op):
        if range_cmp(store_op, self) == 0:
            return True
        else:
            return False

    # Rangeable implementation
    def get_base_address(self):
        return self.address

    # Rangeable implementation
    def get_max_address(self):
        return self.address + self.size

# Fence Operation
class Fence(BaseOperation):
    def __repr__(self):
        return "Fence:" + BaseOperation.__repr__(self)

    def __str__(self):
        return "Fence:" + BaseOperation.__str__(self)

# Lock Operation
class Lock(BaseOperation, Rangeable):
    def __init__(self, address, op_id, tid, src_info):
        self.address = address
        self.size = 8
        BaseOperation.__init__(self, op_id, tid, src_info)

    def __repr__(self):
        return "Lock:" + \
               "addr:" + hex(self.address) + \
               "," + BaseOperation.__str__(self)

    def __str__(self):
        return "Lock:" + \
               "addr:" + hex(self.address) + \
               "," + BaseOperation.__str__(self)

    # Rangeable implementation
    def get_base_address(self):
        return self.address

    # Rangeable implementation
    def get_max_address(self):
        return self.address + self.size

# UnLock Operation
class UnLock(BaseOperation, Rangeable):
    def __init__(self, address, op_id, tid, src_info):
        self.address = address
        self.size = 8
        BaseOperation.__init__(self, op_id, tid, src_info)

    def __repr__(self):
        return "UnLock:" + \
               "addr:" + hex(self.address) + \
               "," + BaseOperation.__str__(self)

    def __str__(self):
        return "UnLock:" + \
               "addr:" + hex(self.address) + \
               "," + BaseOperation.__str__(self)

    # Rangeable implementation
    def get_base_address(self):
        return self.address

    # Rangeable implementation
    def get_max_address(self):
        return self.address + self.size

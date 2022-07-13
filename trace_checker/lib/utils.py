import os
import mmap

# Cacheline size in bytes
CACHELINE_BYTES = 64

# Atomic write size in bytes
ATOMIC_WRITE_BYTES = 8

# Return the cacheline start address
def get_cacheline_address(address):
    return int(address / CACHELINE_BYTES) * CACHELINE_BYTES

class Rangeable:
    """
    Interface for all rangeable objects.

    All rangeable objects must be able to return their base and max
    addresses.
    """
    def get_base_address(self):
        """
        Getter for the base address of the object.

        :return: The base address of the object.
        :rtype: int
        """
        raise NotImplementedError

    def get_max_address(self):
        """
        Getter for the max address of the object.

        :return: The max address of the object.
        :rtype: int
        """
        raise NotImplementedError

def memory_map(filename, size=0, access=mmap.ACCESS_WRITE, offset=0):
    """
    Memory map a file.

    :Warning:
        `offset` has to be a non-negative multiple of PAGESIZE or
        ALLOCATIONGRANULARITY

    :param filename: The file to be mapped.
    :type filename: str
    :param size: Number of bytes to be mapped. If is equal 0, the
        whole file at the moment of the call will be mapped.
    :type size: int
    :param offset: The offset within the file to be mapped.
    :type offset: int
    :param access: The type of access provided to mmap.
    :return: The mapped file.
    :rtype: mmap.mmap
    """
    fd = os.open(filename, os.O_RDWR)
    m_file = mmap.mmap(fd, size, access=access, offset=offset)
    os.close(fd)
    return m_file


def range_cmp(lhs, rhs):
    """
    A range compare function.

    :param lhs: The left hand side of the comparison.
    :type lhs: Rangeable
    :param rhs: The right hand side of the comparison.
    :type rhs: Rangeable
    :return: -1 if lhs is before rhs, 1 when after and 0 on overlap.
    :rtype: int

    The comparison function may be explained as::

        Will return -1:
        |___lhs___|
                        |___rhs___|

        Will return +1:
        |___rhs___|
                        |___lhs___|

        Will return 0:
        |___lhs___|
               |___rhs___|
    """
    if lhs.get_max_address() <= rhs.get_base_address():
        return -1
    elif lhs.get_base_address() >= rhs.get_max_address():
        return 1
    else:
        return 0

# return true if lhs contains rhs
def range_contains(lhs, rhs):
    if lhs.get_base_address() <= rhs.get_base_address() and \
            lhs.get_max_address() >= rhs.get_max_address():
        return True
    else:
        return False

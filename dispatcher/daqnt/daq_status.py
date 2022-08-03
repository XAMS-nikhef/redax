import enum

__all__ = ['DAQ_STATUS']

class DAQ_STATUS(enum.IntEnum):
    IDLE = 0
    ARMING = 1
    ARMED = 2
    RUNNING = 3
    ERROR = 4
    TIMEOUT = 5
    UNKNOWN = 6

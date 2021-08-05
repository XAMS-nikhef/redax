import signal
import threading

__all__ = ['SignalHandler']

class SignalHandler(object):
    """
    A handler to listen for SIGINTs and SIGTERMs from the OS
    """
    def __init__(self):
        self.event = threading.Event()
        signal.signal(signal.SIGINT, self.interrupt)
        signal.signal(signal.SIGTERM, self.interrupt)

    def interrupt(self, *args):
        self.event.set()


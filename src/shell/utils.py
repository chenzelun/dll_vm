from functools import wraps


class Debugger:
    @staticmethod
    def print_all_fields(func):
        @wraps(func)
        def wrapper(obj):
            return str(obj.__dict__)

        return wrapper

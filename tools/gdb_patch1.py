"""
Патч 1 - заменить _DebugExceptionVector в прошивке.
Он находится по адресу 0x40080280 в памяти ESP32.
"""

import elftools

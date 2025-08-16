# Only import Binary Ninja components when running as a plugin
try:
    from binaryninja import *

    # Import plugin components only when Binary Ninja is available
    from . import irre_arch
    from . import irre_view
except ImportError:
    # Binary Ninja not available - this is fine for CLI usage
    pass

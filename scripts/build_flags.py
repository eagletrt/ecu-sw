from SCons.Script import Import

Import("env")

# Adapted for Kraken Nucleo-F767ZI
DRIVER_PATHS = [
    "Drivers/STM32F7xx_HAL_Driver",
    "Drivers/CMSIS",
]

def is_driver(node):
    path = str(node)
    return any(p in path for p in DRIVER_PATHS)

def before_compile(source, target, env_):
    src = source[0]
    if is_driver(src):
        # backup the current CCFLAGS
        env_["_saved_ccflags"] = list(env_.get("CCFLAGS", []))

        # create a new list of flags
        env_["CCFLAGS"] = [
            f for f in env_["_saved_ccflags"]
            if not str(f).startswith("-W")
        ]
        
        # add -w to completely silence this specific driver file
        env_.Append(CCFLAGS=["-w"])

def after_compile(source, target, env_):
    # restore the original flags
    if "_saved_ccflags" in env_:
        env_["CCFLAGS"] = env_.pop("_saved_ccflags")

# Register the actions for all object files
env.AddPreAction("$BUILD_DIR/*.o", before_compile)
env.AddPostAction("$BUILD_DIR/*.o", after_compile)
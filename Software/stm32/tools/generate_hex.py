Import("env", "projenv")

# Custom HEX from ELF
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(" ".join([
        "$OBJCOPY", "-O", "ihex", "-R", ".eeprom",
        "$BUILD_DIR/${PROGNAME}.elf", "$PROJECT_DIR/bin/${PROGNAME}.hex"
    ]), "Building $PROJECT_DIR/bin/${PROGNAME}.hex")
)

env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(" ".join([
        "$OBJCOPY", "-O", "binary", "-R", ".eeprom",
        "$BUILD_DIR/${PROGNAME}.elf", "$PROJECT_DIR/bin/${PROGNAME}.bin"
    ]), "Building $PROJECT_DIR/bin/${PROGNAME}.bin")
)
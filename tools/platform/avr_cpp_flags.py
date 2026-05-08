Import("env")

env.AppendUnique(
    CXXFLAGS=[
        "-fno-threadsafe-statics",
        "-fno-rtti",
    ]
)

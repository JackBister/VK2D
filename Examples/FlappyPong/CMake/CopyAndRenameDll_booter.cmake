
message("CopyAndRenameDll_booter.cmake")

# This is a workaround, the CopyAndRenameDll script needs to be allowed to fail but I couldn't figure it out without this intermediary "_booter" file
execute_process(COMMAND ${CMAKE_COMMAND} -P ../CMake/CopyAndRenameDll.cmake)


message("CopyAndRenameDll.cmake")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# PDBs behave weirdly, so renaming to _old and then deleting does not work as it does for DLLs.
file(GLOB OLD_PDBS ${CMAKE_BINARY_DIR}/bin/*.pdb)
foreach(PDB ${OLD_PDBS})
    message("Removing " ${PDB})
    file(REMOVE ${PDB})
endforeach()

if (EXISTS ${CMAKE_BINARY_DIR}/bin/{% projectName %}_old.dll)
    message("Removing " ${CMAKE_BINARY_DIR}/bin/{% projectName %}_old.dll)
    file(REMOVE ${CMAKE_BINARY_DIR}/bin/{% projectName %}_old.dll)
endif()
if (EXISTS ${CMAKE_BINARY_DIR}/bin/{% projectName %}.dll)
    message("Renaming " ${CMAKE_BINARY_DIR}/bin/{% projectName %}.dll " to " ${CMAKE_BINARY_DIR}/bin/{% projectName %}_old.dll)
    file(RENAME ${CMAKE_BINARY_DIR}/bin/{% projectName %}.dll ${CMAKE_BINARY_DIR}/bin/{% projectName %}_old.dll)
endif()

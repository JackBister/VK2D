
message("CopyAndRenameDll.cmake")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# PDBs behave weirdly, so renaming to _old and then deleting does not work as it does for DLLs.
file(GLOB OLD_PDBS ${CMAKE_BINARY_DIR}/bin/*.pdb)
foreach(PDB ${OLD_PDBS})
    message("Removing " ${PDB})
    file(REMOVE ${PDB})
endforeach()

if (EXISTS ${CMAKE_BINARY_DIR}/bin/FlappyPong_old.dll)
    message("Removing " ${CMAKE_BINARY_DIR}/bin/FlappyPong_old.dll)
    file(REMOVE ${CMAKE_BINARY_DIR}/bin/FlappyPong_old.dll)
endif()
if (EXISTS ${CMAKE_BINARY_DIR}/bin/FlappyPong.dll)
    message("Renaming " ${CMAKE_BINARY_DIR}/bin/FlappyPong.dll " to " ${CMAKE_BINARY_DIR}/bin/FlappyPong_old.dll)
    file(RENAME ${CMAKE_BINARY_DIR}/bin/FlappyPong.dll ${CMAKE_BINARY_DIR}/bin/FlappyPong_old.dll)
endif()

#include <cstdio>

#include <boost/filesystem.hpp>

#if 0
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "clang/Basic/LangOptions.h"
#include "clang/Basic/FileSystemOptions.h"

#include "clang/Basic/SourceManager.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Basic/FileManager.h"

#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"

#include "clang/Lex/Preprocessor.h"
#include "clang/Frontend/CompilerInstance.h"
#endif

void GenerateDirectory_r(boost::filesystem::path& path) {
    using boost::filesystem::directory_iterator;
    using boost::filesystem::is_directory;

    for (auto& de : directory_iterator(path)) {
        boost::filesystem::path subdir = de.path();
        if (is_directory(subdir)) {
            GenerateDirectory_r(subdir);
            continue;
        }
        printf("%s\n", de.path().c_str());
    }
}

int main(int argc, char *argv[]) {
    using boost::filesystem::exists;
    using boost::filesystem::is_directory;
     
    if (argc != 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }
    
    boost::filesystem::path path(argv[1]);
    if (!exists(path) || !is_directory(path)) {
        printf("[ERROR] %s doesn't exist or isn't a directory.\n", argv[1]);
        return 1;
    }

    GenerateDirectory_r(path);
    
    return 0;
}
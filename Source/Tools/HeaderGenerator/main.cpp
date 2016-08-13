#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/tokenizer.hpp"
#include "clang-c/Index.h"

struct Property
{
	//Parent name/class name
	std::string parent;
	std::vector<std::string> attrs;
	bool isPtr = false;
	CXType cxType;
	std::string type;
	std::string name;
};

struct ClassAccumulator
{
	std::vector<Property> properties;
};

struct FileAccumulator
{
	std::string fileName;
	std::unordered_map<std::string, ClassAccumulator> classes;

	void Dump()
	{
		for (auto& cit : classes) {
			printf("Class %s:\n", cit.first.c_str());
			for (auto& pit : cit.second.properties) {
				for (auto& ait : pit.attrs) {
					printf("%s ", ait.c_str());
				}
				printf("%s ", pit.type.c_str());
				if (pit.isPtr) {
					printf("* ");
				}
				printf("%s\n", pit.name.c_str());
			}
		}
	}
};

bool ContainsRead(std::vector<std::string> v)
{
	for (const auto& s : v) {
		if (s == "LuaRead" || s == "LuaReadWrite") {
			return true;
		}
	}
	return false;
}

bool ContainsWrite(std::vector<std::string> v)
{
	for (const auto& s : v) {
		if (s == "LuaWrite" || s == "LuaReadWrite") {
			return true;
		}
	}
	return false;
}

std::string TypeCheck(CXType type)
{
	std::string ts = clang_getCString(clang_getTypeSpelling(type));
	switch (type.kind) {
	case CXType_Bool:
		return "lua_isboolean";
	case CXType_Int:
	case CXType_Float:
	case CXType_Double:
		return "lua_isnumber";
	case CXType_Pointer:
	case CXType_Record:
		return "lua_isuserdata";
	case CXType_Unexposed:
		if (ts == "std::string") {
			return "lua_isstring";
		}
	default:
		printf("[ERROR] Unhandled type %s in TypeCheck.\n", clang_getCString(clang_getTypeKindSpelling(type.kind)));
		return "";
	}
}

std::string TypeCheckFunc(CXType function)
{
	std::stringstream ss;
	int argc = clang_getNumArgTypes(function);
	for (int i = 0; i < argc; ++i) {
		CXType argType = clang_getArgType(function, i);
		//+2 because Lua is 1-indexed and argument 1 must be userdata, checked before this function is called.
		ss << "!" << TypeCheck(argType) << "(L, " << i + 2 << ")";
		if (i != argc - 1) {
			ss << " || ";
		}
	}
	return ss.str();
}

std::string Pull(CXType type)
{
	std::string ts = clang_getCString(clang_getTypeSpelling(type));
	switch (type.kind) {
	case CXType_Bool:
		return "lua_toboolean";
	case CXType_Int:
	case CXType_Float:
	case CXType_Double:
		return "lua_tonumber";
	case CXType_Record:
		return "lua_touserdata";
	case CXType_Unexposed:
		if (ts == "std::string") {
			return "lua_tostring";
		}
	default:
		printf("[ERROR] Unhandled type %s in Pull.\n", clang_getCString(clang_getTypeKindSpelling(type.kind)));
		return "";
	}
}

//TODO: is recursive
std::string Push(Property p)
{
	std::stringstream ss;
	//TODO:
	switch (p.cxType.kind) {
	case CXType_Bool:
		ss << "lua_pushboolean(L, ";
		if (p.isPtr) {
			ss << '*';
		}
		ss << p.name << ");";
		break;
	case CXType_Int:
	case CXType_Float:
	case CXType_Double:
		ss << "lua_pushnumber(L, ";
		if (p.isPtr) {
			ss << '*';
		}
		ss << p.name << ");";
		break;
	case CXType_FunctionProto:
		//TODO: Needs a GenerateCFuncs to run first
		ss << "lua_pushCFunction(L, _" << p.name << "_Lua);";
		break;
	case CXType_LValueReference:
		p.cxType = clang_getPointeeType(p.cxType);
		ss << Push(p);
		break;
	case CXType_Pointer:
		p.cxType = clang_getPointeeType(p.cxType);
		p.isPtr = true;
		ss << Push(p);
		break;
	case CXType_Record:
		ss << p.name;
		if (p.isPtr) {
			ss << "->";
		} else {
			ss << '.';
		}
		ss << "PushToLua(L);";
		break;
	case CXType_Unexposed:
		if (p.type == "std::string") {
			ss << "lua_pushstring(L, " << p.name;
			if (p.isPtr) {
				ss << "->";
			} else {
				ss << '.';
			}
			ss << "c_str());";
			break;
		}
		//Intentional fallthrough
	default:
		printf("[WARNING] Unhandled type %s %s in Push.\n", p.type.c_str(), clang_getCString(clang_getTypeKindSpelling(p.cxType.kind)));
		break;
	}
	return ss.str();
}

std::string GenerateCFunctions(std::string className, ClassAccumulator accumulator)
{
	std::stringstream ss;
	for (auto& pit : accumulator.properties) {
		if (!ContainsRead(pit.attrs)) {
			continue;
		}
		if (pit.cxType.kind == CXType_FunctionProto) {
			//Add one because Lua needs those arguments + the pointer to the object
			int argc = clang_getNumArgTypes(pit.cxType) + 1;
			ss << "int _" << pit.name << "_Lua(lua_State * L) {\n"
				<< "\t int top = lua_gettop(L);\n"
				<< "\t if (top != " << argc << " || !lua_isuserdata(L, 1)" << (argc > 1 ? " || " : "") << TypeCheckFunc(pit.cxType) << ") {\n"
				<< "\t\t lua_pushstring(L, \"Incorrect arguments to " << className << ":" << pit.name << ".\\n\");\n"
				<< "\t\t lua_error(L);\n"
				<< "\t\t return 1;\n"
				<< "\t }\n"
				<< "\t void ** vpp = static_cast<void **>(lua_touserdata(L, 1));\n"
				<< "\t auto ptr = static_cast<" << className << " *>(*vpp);\n";

			for (int i = 1; i < argc; ++i) {
				CXType argType(clang_getArgType(pit.cxType, i - 1));
				if (argType.kind == CXType_Record) {
					ss << "\t auto arg" << i << " = *static_cast<" << clang_getCString(clang_getTypeSpelling(argType)) << " *>(*static_cast<void **>(" << "lua_touserdata(L, " << i + 1 << ")));\n";
					//TODO: Need to change Pull function so this works for abitrary number of pointer indirection
				} else if (argType.kind == CXType_Pointer) {
					CXType pointeeType(clang_getPointeeType(argType));
					ss << "\t auto aarg" << i << " = " << Pull(pointeeType) << "(L, " << i + 1 << ");\n";
					if (pointeeType.kind == CXType_Record) {
						ss << "\t auto arg" << i << " = *static_cast<" << clang_getCString(clang_getTypeSpelling(pointeeType)) << " **>(aarg" << i << ");\n";
					} else {
						ss << "\t auto arg" << i << " = &aarg" << i << ";\n";
					}
				} else {
					ss << "\t auto arg" << i << " = " << Pull(argType) << "(L, " << i + 1 << ");\n";
				}
			}
			Property ret;
			ret.cxType = clang_getResultType(pit.cxType);
			ret.name = "res";
			ret.isPtr = false;
			ret.type = clang_getCString(clang_getTypeSpelling(ret.cxType));

			ss << "\t auto res = " << pit.name << "(";
			for (int i = 1; i < argc; ++i) {
				ss << "arg" << i;
				if (i != argc - 1) {
					ss << ", ";
				}
			}

			ss << ");\n"
				<< "\t " << Push(ret) << "\n"
				<< "}\n\n";
		}
	}
	return ss.str();
}

std::string GenerateLuaIndex(std::string className, ClassAccumulator accumulator)
{
	std::stringstream ss;
	ss << "int " << className << "::LuaIndex(lua_State * L) {\n"
		<< "\t const char * idx = lua_tostring(L, 2);\n"
		<< "\t if (idx == nullptr) {\n"
		<< "\t\t lua_pushnil(L);\n"
		<< "\t\t return 1;\n"
		<< "\t }\n\t";

	for (auto& pit : accumulator.properties) {
		if (!ContainsRead(pit.attrs)) {
			continue;
		}
		ss << " if (strcmp(\"" << pit.name << "\", idx) == 0) {\n"
			<< "\t\t " << Push(pit) << "\n"
			<< "\t } else";
	}
	ss << " {\n"
		<< "\t\t lua_pushnil(L);\n"
		<< "\t }\n"
		<< "\t return 1;\n"
		<< "}\n\n";
	return ss.str();
}

std::string GenerateLuaNewIndex(std::string className, ClassAccumulator accumulator)
{
	std::stringstream ss;
	ss << "int " << className << "::LuaNewIndex(lua_State * L) {\n"
		<< "\t if (lua_gettop(L) != 3) {\n"
		<< "\t\t return 0;\n"
		<< "\t }\n"
		<< "\t const char * idx = lua_tostring(L, 2);\n"
		<< "\t if (idx == nullptr) {\n"
		<< "\t\t return 0;\n"
		<< "\t }\n\t";

	for (auto& pit : accumulator.properties) {
		if (!ContainsWrite(pit.attrs)) {
			continue;
		}
		ss << " if (strcmp(\"" << pit.name << "\", idx) == 0) {\n"
			<< "\t\t if (!" << TypeCheck(pit.cxType) << "(L, 3)) {\n"
			<< "\t\t\t return 0;\n"
			<< "\t\t }\n";
		if (pit.cxType.kind == CXType_Record) {
			ss << "\t\t " << pit.name << " = **static_cast<" << pit.type << " **>(lua_touserdata(L, 3));\n";
		} else {
			ss << "\t\t " << pit.name << " = " << Pull(pit.cxType) << "(L, 3);\n";
		}
		ss << "\t } else";
	}
	ss << " { }\n"
		<< "\t return 0;\n"
		<< "}\n\n";
	return ss.str();
}

std::vector<std::string> ParseAttributes(const char * const s)
{
	std::vector<std::string> ret;
	std::string ss(s);
	boost::tokenizer<> tokenizer(ss);

	for (auto& it : tokenizer) {
		ret.push_back(it);
	}
	return ret;
}

CXChildVisitResult AttributeVisitor(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
	if (clang_getCursorKind(cursor) == CXCursor_AnnotateAttr) {
		CXString s = clang_getCursorSpelling(cursor);
		auto prop = static_cast<Property *>(clientData);
		prop->attrs = ParseAttributes(clang_getCString(s));
		clang_disposeString(s);
	}
	return CXChildVisit_Recurse;
}

CXChildVisitResult DeclarationVisitor(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
	if (!clang_Location_isFromMainFile(clang_getCursorLocation(cursor))) {
		return CXChildVisit_Continue;
	}
	if (clang_isDeclaration(clang_getCursorKind(cursor))) {
		Property p;
		CXString ps = clang_getCursorSpelling(parent);
		p.parent = std::string(clang_getCString(ps));
		clang_visitChildren(cursor, AttributeVisitor, &p);

		if (p.attrs.size() == 0) {
			return CXChildVisit_Recurse;
		}
		CXString s = clang_getCursorSpelling(cursor);
		p.name = std::string(clang_getCString(s));
		clang_disposeString(s);
		CXType type = clang_getCursorType(cursor);
		if (type.kind == CXType_Pointer) {
			p.isPtr = true;
			type = clang_getPointeeType(type);
		}
		CXString ts = clang_getTypeSpelling(type);
		p.cxType = type;
		p.type = std::string(clang_getCString(ts));
		clang_disposeString(ts);
		auto fileAccumulator = static_cast<FileAccumulator *>(clientData);
		if (p.parent == fileAccumulator->fileName) {
			//TODO:
			//For some reason the class declaration gets the annotations of all its children, meaning one "property" with type = name and parent = filename will be created
			//This is a band aid fix for it
			return CXChildVisit_Recurse;
		}
		if (fileAccumulator->classes.count(p.parent) == 0) {
			fileAccumulator->classes[p.parent] = ClassAccumulator();
		}
		fileAccumulator->classes[clang_getCString(ps)].properties.push_back(p);
		clang_disposeString(ps);
	}
	return CXChildVisit_Recurse;
}

void ProcessFile(const boost::filesystem::path& path)
{
	CXIndex index = clang_createIndex(1, 0);
	if (!index) {
		printf("[ERROR] HeaderGenerator: Failed to create index.\n");
		exit(1);
	}
	const char * const args[] = {
		//TODO: God damn it.
		"-IC:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/5.3.0/include",
		"-IC:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/5.3.0/../../../../include",
		"-IC:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/5.3.0/include-fixed",
		"-IC:/msys64/mingw64/bin/../lib/gcc/x86_64-w64-mingw32/5.3.0/../../../../x86_64-w64-mingw32/include",
		"-IC:/msys64/mingw64/lib/gcc/../../include/c++/5.3.0",
		"-IC:/msys64/mingw64/lib/gcc/../../include/c++/5.3.0/x86_64-w64-mingw32",
		"-IC:/msys64/mingw64/lib/gcc/../../include/c++/5.3.0/backward",
		"-isystem /mingw64/include/c++/5.3.0",
		"-Iinclude",
		"-ISource",
		"-ISource/Core",
		"-ISource/Core/Components",
		"-ISource/Core/Lua",
		"-ISource/Core/Math",
		"-ISource/Core/Rendering",
		"-std=c++14",
		"-DHEADERGENERATOR"
	};
	CXTranslationUnit unit = clang_parseTranslationUnit(index, path.generic_string().c_str(), args, sizeof(args)/sizeof(args[0]), nullptr, 0, CXTranslationUnit_None);
	CXCursor cursor = clang_getTranslationUnitCursor(unit);

	FileAccumulator res;
	res.fileName = path.generic_string();
	clang_visitChildren(cursor, DeclarationVisitor, &res);
//	res.Dump();
	std::stringstream fileNameSS;
	fileNameSS << path.generic_string() << ".generated.h";
	std::fstream outStream(fileNameSS.str(), std::ios_base::out);
	outStream << "#include \"" << path.filename().generic_string() << "\"\n\n";
	for (auto& it : res.classes) {
		outStream << GenerateCFunctions(it.first, it.second)
			<< GenerateLuaIndex(it.first, it.second)
			<< GenerateLuaNewIndex(it.first, it.second);
		/*
		printf("%s\n", GenerateCFunctions(it.first, it.second).c_str());
		printf("%s\n", GenerateLuaIndex(it.first, it.second).c_str());
		printf("%s\n", GenerateLuaNewIndex(it.first, it.second).c_str());
		*/
	}
	outStream.close();
	clang_disposeTranslationUnit(unit);
	clang_disposeIndex(index);
}

void Generate_r(const boost::filesystem::path& path) {
    using boost::filesystem::directory_iterator;
    using boost::filesystem::is_directory;
	using boost::filesystem::is_regular_file;

	if (is_regular_file(path)) {
		//TODO: Cannot use path::c_str because on Windows it results in wchar_t which doesn't work with %s.
		printf("Processing %s.\n", path.generic_string().c_str());
		ProcessFile(path);
		return;
	}

    for (auto& de : directory_iterator(path)) {
        const boost::filesystem::path subdir = de.path();
        if (is_directory(subdir)) {
            Generate_r(subdir);
		} else {
			printf("[WARNING] path is not directory or regular file: %s\n", subdir.generic_string().c_str());
		}
	}
}

int main(const int argc, const char *argv[]) {
    using boost::filesystem::exists;

    if (argc != 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    const boost::filesystem::path path(argv[1]);
    if (!exists(path)) {
        printf("[ERROR] %s doesn't exist.\n", argv[1]);
        return 1;
    }

    Generate_r(path);
    
    return 0;
}
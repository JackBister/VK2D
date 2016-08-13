#pragma once

#ifdef HEADERGENERATOR
#define PROPERTY(...) __attribute__((annotate(#__VA_ARGS__)))
#else
#define PROPERTY(...)
#endif

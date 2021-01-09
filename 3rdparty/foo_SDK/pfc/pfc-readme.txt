PFC : Peter's Foundation Classes

A library of loosely connected classes used by foobar2000 codebase; freely available and reusable for other projects.

PFC is not state-of-art code. Many parts of it exist only to keep old bits of foobar2000 codebase ( also third party foobar2000 components ) compiling without modification. For an example, certain classes predating 'pfc' namespace use exist outside the namespace.

Regarding build configurations-
"Release FB2K" and "Debug FB2K" suppress the compilation of pfc-fb2k-hooks.cpp - which allows relevant calls to be redirected to shared.dll
These configurations should be used when compiling fb2k components. Regular configurations should be used for non fb2k apps instead.
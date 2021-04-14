

```
In file included from DerivedSources/JavaScriptCore/unified-sources/UnifiedSource122.cpp:8:
../../Source/JavaScriptCore/tools/JSDollarVM.cpp:180:37: error: reference to 'Handle' is ambiguous
    bool isReachableFromOpaqueRoots(Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor, const char** reason) override
                                    ^
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.1.sdk/usr/include/MacTypes.h:249:41: note: candidate found by name lookup is 'Handle'
typedef Ptr *                           Handle;
                                        ^
../../Source/JavaScriptCore/heap/Handle.h:109:29: note: candidate found by name lookup is 'JSC::Handle'
template <typename T> class Handle : public HandleBase, public HandleConverter<Handle<T>, T> {
                            ^
1 error generated.

```

```
>>> Tools/gtk/install-dependencies
Error: Running Homebrew as root is extremely dangerous and no longer supported.
As Homebrew does not drop privileges on installation you would be giving all
build scripts full access to your system.
```
#ifndef CLIENTVTABLEBYNAME_H
#define CLIENTVTABLEBYNAME_H

// ============================================================================
//  Resolve Steam client internal-interface methods BY NAME at runtime.
//
//  Why: interfaces like IClientEngine and IClientRemoteClientManager are
//  undocumented, and Valve inserts / removes / reorders their methods between
//  Steam client builds. Because a C++ virtual call is compiled to a fixed
//  vtable *index* taken from the declaration order in the OpenSteamworks
//  headers, a single drifted build makes every call after the inserted method
//  land on the wrong function -> wrong arguments -> crash. This is the usual
//  "worked last month, crashes today" report.
//
//  Fix: each Steam client method stub references its own bare name as an ASCII
//  string near the top of the function (used by Valve's telemetry / profiling
//  scopes). We disassemble just far enough to read the `lea reg, [rip+disp32]`
//  that loads that string, and build a name -> vtable-index map at runtime.
//  Calls then go through the resolved index, so they self-adapt to whatever
//  build is installed.
//
//  Cross-platform: the scan is pure x86-64 machine-code pattern matching and
//  the string is present in both steamclient64.dll (Windows / PE) and
//  steamclient.so (Linux / ELF, incl. Steam Deck), so the same code path works
//  on every target this project builds for. Module bounds are found with the
//  OS loader (PE headers via VirtualQuery on Windows; dladdr + dl_iterate_phdr
//  on Linux). Only x86-64 is handled; that is the only arch the Steam client
//  ships as.
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cctype>
#include <string>
#include <unordered_map>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <link.h>
#endif

namespace clientvt
{
    // --- locate the [lo, hi) address range of the module that owns `addr` ----
#if defined(_WIN32)
    inline bool ModuleRangeOf(const void* addr, uintptr_t& lo, uintptr_t& hi)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(addr, &mbi, sizeof(mbi)))
            return false;

        uintptr_t base = reinterpret_cast<uintptr_t>(mbi.AllocationBase);
        if (!base)
            return false;

        const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return false;

        const IMAGE_NT_HEADERS* nt =
            reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return false;

        lo = base;
        hi = base + nt->OptionalHeader.SizeOfImage;
        return true;
    }
#else
    struct PhdrQuery { uintptr_t base; uintptr_t span; };

    inline int PhdrCb(struct dl_phdr_info* info, size_t, void* data)
    {
        PhdrQuery* q = static_cast<PhdrQuery*>(data);
        if (static_cast<uintptr_t>(info->dlpi_addr) != q->base)
            return 0; // not the object we want; keep iterating

        uintptr_t maxEnd = 0;
        for (int i = 0; i < info->dlpi_phnum; ++i)
        {
            const ElfW(Phdr)& ph = info->dlpi_phdr[i];
            if (ph.p_type == PT_LOAD)
            {
                uintptr_t end = static_cast<uintptr_t>(ph.p_vaddr + ph.p_memsz);
                if (end > maxEnd)
                    maxEnd = end;
            }
        }
        q->span = maxEnd;
        return 1; // found it; stop
    }

    inline bool ModuleRangeOf(const void* addr, uintptr_t& lo, uintptr_t& hi)
    {
        Dl_info di;
        std::memset(&di, 0, sizeof(di));
        if (!dladdr(const_cast<void*>(addr), &di) || !di.dli_fbase)
            return false;

        lo = reinterpret_cast<uintptr_t>(di.dli_fbase);
        PhdrQuery q = { lo, 0 };
        dl_iterate_phdr(PhdrCb, &q);
        if (q.span == 0)
            return false;

        hi = lo + q.span;
        return true;
    }
#endif

    // --- read the bare method name a stub references via `lea reg,[rip+d32]` --
    inline std::string NameOf(uintptr_t fn, uintptr_t lo, uintptr_t hi)
    {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(fn);
        for (int k = 0; k < 220; ++k)
        {
            if (fn + k + 7 >= hi) // don't read past the module
                break;

            uint8_t rex = p[k];
            if (rex != 0x48 && rex != 0x4C && rex != 0x49 && rex != 0x4D)
                continue;                       // REX.W prefix
            if (p[k + 1] != 0x8D)
                continue;                       // LEA opcode
            if ((p[k + 2] & 0xC7) != 0x05)
                continue;                       // ModRM: mod=00, r/m=101 (RIP-relative)

            int32_t disp;
            std::memcpy(&disp, p + k + 3, sizeof(disp));
            uintptr_t target = fn + k + 7 + static_cast<int64_t>(disp);
            if (target < lo || target >= hi)
                continue;

            const char* s = reinterpret_cast<const char*>(target);
            std::string out;
            bool bad = false;
            for (int j = 0; j < 96; ++j)
            {
                char c = s[j];
                if (c == 0)
                    break;
                if (c < 32 || c > 126) { bad = true; break; }
                out.push_back(c);
            }
            if (bad)
                continue;

            // A method name is a single C++ identifier. Requiring that (instead
            // of merely "no spaces") rejects the __FILE__ path and module-name
            // strings some stubs reference; we keep scanning past those until we
            // reach a real identifier.
            if (out.size() >= 6)
            {
                char c0 = out[0];
                bool ok = (c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z') || c0 == '_';
                int letters = 0;
                for (size_t i = 0; i < out.size() && ok; ++i)
                {
                    char c = out[i];
                    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                          || (c >= '0' && c <= '9') || c == '_'))
                        ok = false;
                    else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                        ++letters;
                }
                if (ok && letters >= 4)
                    return out;
            }
        }
        return std::string();
    }

    // --- build name -> vtable index for a COM-like interface pointer ----------
    inline std::unordered_map<std::string, int> ScanVTableNames(void* iface)
    {
        std::unordered_map<std::string, int> map;
        if (!iface)
            return map;

        void** vt = *reinterpret_cast<void***>(iface);
        uintptr_t lo = 0, hi = 0;
        if (!ModuleRangeOf(reinterpret_cast<void*>(vt[0]), lo, hi))
            return map;

        for (int i = 0; i < 400; ++i)
        {
            uintptr_t fn = reinterpret_cast<uintptr_t>(vt[i]);
            if (fn < lo || fn >= hi) // ran off the end of the vtable
                break;
            std::string name = NameOf(fn, lo, hi);
            if (!name.empty() && map.find(name) == map.end())
                map[name] = i;
        }
        return map;
    }

    // --- a bound interface you can call by method name ------------------------
    //
    // On x86-64 every calling convention (Windows x64 and System V) passes
    // `this` as the first integer argument, so invoking the resolved function
    // pointer as a free function with the interface pointer prepended is ABI-
    // identical to the original virtual call. Argument structs are forwarded by
    // value with their declared types, so their passing (registers vs memory)
    // matches too.
    class NamedVTable
    {
    public:
        NamedVTable() : m_iface(NULL) {}

        void Bind(void* iface)
        {
            m_iface = iface;
            m_names = iface ? ScanVTableNames(iface)
                            : std::unordered_map<std::string, int>();
        }

        bool Ok() const { return m_iface != NULL && !m_names.empty(); }
        bool Has(const char* name) const { return m_names.find(name) != m_names.end(); }
        size_t Count() const { return m_names.size(); }

        // Resolved vtable index for a method, or -1 if its stub didn't embed a
        // name (a few don't). Such a method can still be reached by anchoring to
        // a named neighbour; see CancelRemotePlayInviteAndSession in SteamStuff.h.
        int IndexOf(const char* name) const
        {
            std::unordered_map<std::string, int>::const_iterator it = m_names.find(name);
            return (it != m_names.end()) ? it->second : -1;
        }

        // Call the named method. The caller is responsible for checking Has()
        // for anything that might legitimately be missing; a name that is known
        // to exist (verified once at startup) can be called directly.
        template <class Ret>
        Ret Invoke(const char* name) const { return InvokeAt<Ret>(IndexOf(name)); }
        template <class Ret, class A0>
        Ret Invoke(const char* name, A0 a0) const { return InvokeAt<Ret>(IndexOf(name), a0); }
        template <class Ret, class A0, class A1>
        Ret Invoke(const char* name, A0 a0, A1 a1) const { return InvokeAt<Ret>(IndexOf(name), a0, a1); }

        // Call by explicit (already resolved) vtable index.
        template <class Ret>
        Ret InvokeAt(int idx) const
        {
            typedef Ret (*Fn)(void*);
            return reinterpret_cast<Fn>(SlotAt(idx))(m_iface);
        }
        template <class Ret, class A0>
        Ret InvokeAt(int idx, A0 a0) const
        {
            typedef Ret (*Fn)(void*, A0);
            return reinterpret_cast<Fn>(SlotAt(idx))(m_iface, a0);
        }
        template <class Ret, class A0, class A1>
        Ret InvokeAt(int idx, A0 a0, A1 a1) const
        {
            typedef Ret (*Fn)(void*, A0, A1);
            return reinterpret_cast<Fn>(SlotAt(idx))(m_iface, a0, a1);
        }

    protected:
        void* SlotAt(int idx) const
        {
            void** vt = *reinterpret_cast<void***>(m_iface);
            return vt[idx < 0 ? 0 : idx]; // callers verify idx; clamp defensively
        }

        void* m_iface;
        std::unordered_map<std::string, int> m_names;
    };
}

#endif // CLIENTVTABLEBYNAME_H

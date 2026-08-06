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
//  on every target this project builds for. Only x86-64 is handled; that is the
//  only arch the Steam client ships as.
//
//  Memory safety: a Windows module is one contiguous committed image, but an
//  ELF object is several PT_LOAD segments (text / rodata / data) with UNMAPPED
//  gaps between them. So we never scan against one big [base, base+size) span;
//  instead we collect the individual mapped segments and confine every read to
//  the segment that owns the address. Reading across a gap would SIGSEGV (and
//  on Linux that takes the whole Steam session down) — this is why an earlier
//  single-span version crashed on Linux while working on Windows.
// ============================================================================

#include <cstdint>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>
#include <vector>
#include <unordered_map>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <link.h>
#endif

namespace clientvt
{
    // Opt-in diagnostics: set env RPW_VTDEBUG=1 to log the resolver's view of
    // the module (segments, scan progress, resolved names/indices) to stderr.
    // Off by default and effectively zero cost.
    inline bool DebugEnabled()
    {
        static int on = -1;
        if (on < 0)
            on = (std::getenv("RPW_VTDEBUG") != NULL) ? 1 : 0;
        return on != 0;
    }

    inline void Log(const char* fmt, ...)
    {
        if (!DebugEnabled())
            return;
        va_list ap;
        va_start(ap, fmt);
        std::vfprintf(stderr, fmt, ap);
        va_end(ap);
        std::fflush(stderr);
    }

    // A single mapped, readable address range within a module.
    struct Segment { uintptr_t lo; uintptr_t hi; };

    // Is [a, a+n) fully inside one mapped segment?  Reads must pass this so we
    // never touch an unmapped inter-segment gap.
    inline bool Readable(const std::vector<Segment>& segs, uintptr_t a, size_t n)
    {
        for (size_t i = 0; i < segs.size(); ++i)
            if (a >= segs[i].lo && a + n <= segs[i].hi)
                return true;
        return false;
    }

    // Upper bound of the segment that owns `a` (so a forward scan starting at a
    // known-good address can't walk past that segment into a gap). 0 if none.
    inline uintptr_t SegmentEndOf(const std::vector<Segment>& segs, uintptr_t a)
    {
        for (size_t i = 0; i < segs.size(); ++i)
            if (a >= segs[i].lo && a < segs[i].hi)
                return segs[i].hi;
        return 0;
    }

    // --- collect the mapped segments of the module that owns `addr` -----------
#if defined(_WIN32)
    inline bool ModuleSegments(const void* addr, std::vector<Segment>& segs)
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

        // A PE image is mapped as one contiguous committed region, so a single
        // [base, base+SizeOfImage) segment is both correct and safe to read.
        Segment s;
        s.lo = base;
        s.hi = base + nt->OptionalHeader.SizeOfImage;
        segs.push_back(s);
        Log("[vt] module base=%p size=0x%zx (1 contiguous PE image)\n",
            reinterpret_cast<void*>(base),
            static_cast<size_t>(nt->OptionalHeader.SizeOfImage));
        return true;
    }
#else
    struct SegQuery { uintptr_t base; std::vector<Segment>* segs; };

    inline int SegCb(struct dl_phdr_info* info, size_t, void* data)
    {
        SegQuery* q = static_cast<SegQuery*>(data);
        if (static_cast<uintptr_t>(info->dlpi_addr) != q->base)
            return 0; // not the object we want; keep iterating

        for (int i = 0; i < info->dlpi_phnum; ++i)
        {
            const ElfW(Phdr)& ph = info->dlpi_phdr[i];
            if (ph.p_type != PT_LOAD)
                continue;
            Segment s;
            s.lo = q->base + ph.p_vaddr;
            s.hi = s.lo + ph.p_memsz; // p_memsz covers bss too (mapped anon)
            q->segs->push_back(s);
        }
        return 1; // found it; stop
    }

    inline bool ModuleSegments(const void* addr, std::vector<Segment>& segs)
    {
        Dl_info di;
        std::memset(&di, 0, sizeof(di));
        if (!dladdr(const_cast<void*>(addr), &di) || !di.dli_fbase)
            return false;

        SegQuery q;
        q.base = reinterpret_cast<uintptr_t>(di.dli_fbase);
        q.segs = &segs;
        dl_iterate_phdr(SegCb, &q);

        if (DebugEnabled())
        {
            Log("[vt] module %s base=%p : %zu PT_LOAD segment(s)\n",
                di.dli_fname ? di.dli_fname : "?",
                di.dli_fbase, segs.size());
            for (size_t i = 0; i < segs.size(); ++i)
                Log("[vt]   seg[%zu] %p .. %p\n", i,
                    reinterpret_cast<void*>(segs[i].lo),
                    reinterpret_cast<void*>(segs[i].hi));
        }
        return !segs.empty();
    }
#endif

    // --- read the bare method name a stub references via `lea reg,[rip+d32]` --
    inline std::string NameOf(uintptr_t fn, const std::vector<Segment>& segs)
    {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(fn);

        // Confine the instruction scan to the code segment that owns `fn`, so we
        // can't walk off the end of .text into an unmapped gap.
        uintptr_t codeHi = SegmentEndOf(segs, fn);
        if (!codeHi)
            return std::string();

        // Scan window (bytes into the stub). The name-loading `lea` sits near the
        // top of the function, but on Linux (steamclient.so) the telemetry-scope
        // string is emitted deeper in the prologue than on Windows, so a tight cap
        // misses it for some methods and they resolve to -1. 300 covers both; the
        // window is only an upper bound — the per-segment guard below stops the
        // scan at the real end of .text, so a larger cap is free and safe.
        for (int k = 0; k < 300; ++k)
        {
            if (fn + k + 7 > codeHi) // instruction bytes must stay in this segment
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

            // The string may live in a different segment (rodata); cap the read
            // to whatever segment owns `target` so we never cross a gap.
            uintptr_t strHi = SegmentEndOf(segs, target);
            if (!strHi)
                continue;

            const char* s = reinterpret_cast<const char*>(target);
            std::string out;
            bool bad = false;
            for (int j = 0; j < 96 && target + j < strHi; ++j)
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

        std::vector<Segment> segs;
        if (!ModuleSegments(reinterpret_cast<void*>(vt[0]), segs))
        {
            Log("[vt] ModuleSegments failed for vt[0]=%p\n",
                reinterpret_cast<void*>(vt[0]));
            return map;
        }

        // The vtable array itself lives in a segment; don't read entries past
        // the end of that segment (another potential cross-gap read on Linux).
        uintptr_t vtHi = SegmentEndOf(segs, reinterpret_cast<uintptr_t>(vt));

        for (int i = 0; i < 400; ++i)
        {
            uintptr_t slot = reinterpret_cast<uintptr_t>(&vt[i]);
            if (vtHi && slot + sizeof(void*) > vtHi) // ran off the vtable's segment
                break;

            uintptr_t fn = reinterpret_cast<uintptr_t>(vt[i]);
            if (!SegmentEndOf(segs, fn)) // slot doesn't point into the module -> end of vtable
                break;

            std::string name = NameOf(fn, segs);
            if (!name.empty() && map.find(name) == map.end())
                map[name] = i;
        }
        Log("[vt] resolved %zu named method(s)\n", map.size());
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

        // Dump the full resolved map (only when RPW_VTDEBUG is set). Handy for a
        // tester to paste back so drift on their build can be diagnosed.
        void DebugDump() const
        {
            if (!DebugEnabled())
                return;
            Log("[vt] --- %zu resolved methods ---\n", m_names.size());
            for (std::unordered_map<std::string, int>::const_iterator it = m_names.begin();
                 it != m_names.end(); ++it)
                Log("[vt]   %-48s -> %d\n", it->first.c_str(), it->second);
        }

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

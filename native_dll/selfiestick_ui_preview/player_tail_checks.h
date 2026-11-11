#pragma once
#include <thread>

// Execute the production tail stub in this EXE only. Native mutations happen
// AFTER the early callback; original tail copies happen AFTER the late callback.
namespace player_tail_checks {
inline float nativeFov{90};
inline selfiestick::Vec3 beforeNative{};
inline void __fastcall NativeMutation(void* view) {
    using namespace selfiestick;
    TryReadViewSetupOrigin(view, beforeNative);
    hand_checks::Store(view, kViewSetupOriginOffset, Vec3{-1000, 2000, -300});
    hand_checks::Store(view, kViewSetupAnglesOffset, Vec3{20, 40, 0});
    hand_checks::Store(view, kViewSetupFovOffset, nativeFov);
}

struct Harness {
    unsigned char* page{};
    bool ready{};
    Harness() {
        using namespace selfiestick;
        page = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!page) return;
        std::vector<std::uint8_t> code;
        const auto bytes = [&](std::initializer_list<std::uint8_t> data) { code.insert(code.end(), data); };
        const auto u32 = [&](std::uint32_t v) { for (int i=0;i<4;++i) code.push_back(static_cast<std::uint8_t>(v>>(i*8))); };
        const auto u64 = [&](std::uint64_t v) { for (int i=0;i<8;++i) code.push_back(static_cast<std::uint8_t>(v>>(i*8))); };
        const auto call = [&](void* target) { bytes({0x48,0x89,0xD9,0x48,0xB8});u64(reinterpret_cast<std::uintptr_t>(target));bytes({0xFF,0xD0}); };
        constexpr unsigned regs[]{1,2,8,9,10};
        bytes({0x53,0x55,0x56,0x48,0x81,0xEC});u32(0xE0);
        bytes({0x48,0x89,0xCB,0x48,0x8D,0xB3});u32(0x4A0);
        bytes({0x48,0x8D,0xAB});u32(0x4B8);
        bytes({0x0F,0x29,0xB4,0x24});u32(0xB0); // save caller's XMM6 for native replay
        call(reinterpret_cast<void*>(&SelfieStickSetUpViewBridge));
        call(reinterpret_cast<void*>(&NativeMutation));
        for (unsigned i=0;i<5;++i) {
            const unsigned r=regs[i];bytes({static_cast<std::uint8_t>(r>=8?0x49:0x48),static_cast<std::uint8_t>(0xB8+(r&7))});
            u64(0x1122334455667700ull+i);
        }
        for (unsigned r=1;r<=5;++r) {
            std::memset(page+0xC00+r*16, static_cast<int>(0x10+r), 16);
            bytes({0x48,0xB8});u64(reinterpret_cast<std::uintptr_t>(page+0xC00+r*16));
            bytes({0xF3,0x0F,0x6F,static_cast<std::uint8_t>(r<<3)});
        }
        const auto jump=code.size(); bytes({0xE9});u32(0);
        const std::int32_t rel=static_cast<std::int32_t>(0x400-(jump+5)); std::memcpy(code.data()+jump+1,&rel,4);
        const auto continuation=code.size();
        for (unsigned i=0;i<5;++i) {
            const unsigned r=regs[i];bytes({static_cast<std::uint8_t>(r>=8?0x4C:0x48),0x89,static_cast<std::uint8_t>(0x83|((r&7)<<3))});u32(0x700+i*8);
        }
        for (unsigned r=1;r<=5;++r) { bytes({0xF3,0x0F,0x7F,static_cast<std::uint8_t>(0x83|(r<<3))});u32(0x740+(r-1)*16); }
        // Same final origin/angle copies as the installed native function.
        bytes({0xF2,0x0F,0x11,0x83});u32(0x554);
        bytes({0x89,0x83});u32(0x55C);
        bytes({0xF2,0x0F,0x10,0x45,0x00,0xF2,0x0F,0x11,0x83});u32(0x560);
        bytes({0x8B,0x45,0x08,0x89,0x83});u32(0x568);
        bytes({0x48,0x81,0xC4});u32(0xE0);bytes({0x5E,0x5D,0x5B,0xC3});
        if (code.size()>=0x400) return;
        std::memcpy(page,code.data(),code.size());
        // Callback thunk deliberately clobbers volatile registers AFTER invoking
        // real production code, proving the stub restores live machine state.
        code.clear();bytes({0x48,0x83,0xEC,0x28,0x48,0xB8});u64(reinterpret_cast<std::uintptr_t>(&SelfieStickSetUpViewTailBridge));bytes({0xFF,0xD0});
        for (unsigned r=1;r<=5;++r) {
            std::memset(page+0xD00+r*16, static_cast<int>(0x80+r), 16);
            bytes({0x48,0xB8});u64(reinterpret_cast<std::uintptr_t>(page+0xD00+r*16));
            bytes({0xF3,0x0F,0x6F,static_cast<std::uint8_t>(r<<3)});
        }
        for (unsigned r:regs) { bytes({static_cast<std::uint8_t>(r>=8?0x49:0x48),static_cast<std::uint8_t>(0xB8+(r&7))});u64(0xFEE00000ull+r); }
        bytes({0x48,0x83,0xC4,0x28,0xC3});std::memcpy(page+0x800,code.data(),code.size());
        std::string failure;
        if (!hook::BuildPlayerTailDetourStub(reinterpret_cast<std::uintptr_t>(page+0x400),
            reinterpret_cast<std::uintptr_t>(page+continuation), reinterpret_cast<std::uintptr_t>(page+0x800),
            kPlayerTailReplay, code, &failure)) return;
        std::memcpy(page+0x400,code.data(),code.size());
        auto invalid=kPlayerTailReplay;invalid[0]^=1;
        std::vector<std::uint8_t> rejected;
        if (hook::BuildPlayerTailDetourStub(reinterpret_cast<std::uintptr_t>(page+0x400),
            reinterpret_cast<std::uintptr_t>(page+continuation), reinterpret_cast<std::uintptr_t>(page+0x800),
            invalid,rejected,&failure)) return;
        FlushInstructionCache(GetCurrentProcess(),page,4096);ready=true;
    }
    bool Run(void* view, float fov) {
        if (!ready) return false; nativeFov=fov;
        reinterpret_cast<void(__fastcall*)(void*)>(page)(view);
        for (unsigned i=0;i<5;++i) {
            std::uint64_t value{};
            std::memcpy(&value,static_cast<unsigned char*>(view)+0x700+i*8,8);
            if (value!=0x1122334455667700ull+i) return false;
        }
        for (unsigned r=1;r<=5;++r) for (unsigned i=0;i<16;++i)
            if (static_cast<unsigned char*>(view)[0x740+(r-1)*16+i]!=0x10+r) return false;
        return true;
    }
    ~Harness() { if (page) VirtualFree(page,0,MEM_RELEASE); }
};
}

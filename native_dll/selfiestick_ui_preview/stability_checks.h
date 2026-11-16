#pragma once
#include <thread>
#include <future>

namespace stability_checks {
using namespace selfiestick;
enum class Failure { None, Open, Enumerate, Suspend, Context, Busy, Write };
inline Failure failure{};
inline int enumerated{}, closed{}, suspended{}, resumed{}, writes{};
inline bool writeBeforeSafe{};
HANDLE WINAPI Snapshot(DWORD, DWORD) { return reinterpret_cast<HANDLE>(100); }
BOOL WINAPI First(HANDLE, LPTHREADENTRY32 entry) {
    enumerated = 1; entry->th32OwnerProcessID = GetCurrentProcessId();
    entry->th32ThreadID = GetCurrentThreadId() + 1; return TRUE;
}
BOOL WINAPI Next(HANDLE, LPTHREADENTRY32 entry) {
    if (enumerated++ == 1) { ++entry->th32ThreadID; return TRUE; }
    return FALSE;
}
HANDLE WINAPI Open(DWORD, BOOL, DWORD id) {
    return failure == Failure::Open && enumerated == 2 ? nullptr : reinterpret_cast<HANDLE>(static_cast<uintptr_t>(id));
}
DWORD WINAPI Suspend(HANDLE) {
    if (failure == Failure::Suspend && suspended == 1) return static_cast<DWORD>(-1);
    ++suspended; return 0;
}
DWORD WINAPI Resume(HANDLE) { ++resumed; return 1; }
BOOL WINAPI Context(HANDLE, LPCONTEXT context) {
    if (failure == Failure::Context) return FALSE;
    context->Rip = failure == Failure::Busy ? 0x12340000 : 0; return TRUE;
}
BOOL WINAPI Close(HANDLE) { ++closed; return TRUE; }
DWORD WINAPI Error() { return failure == Failure::Enumerate ? ERROR_ACCESS_DENIED : ERROR_NO_MORE_FILES; }
void WINAPI SleepNoop(DWORD) {}
bool Write(void*, const void*, std::size_t) {
    ++writes; writeBeforeSafe |= suspended - resumed != 2;
    return failure != Failure::Write;
}
bool PatchFailures() {
    PatchThreadApi api{};
    api.snapshot=Snapshot; api.first=First; api.next=Next; api.open=Open;
    api.suspend=Suspend; api.resume=Resume; api.context=Context; api.close=Close;
    api.error=Error; api.sleep=SleepNoop; api.write=Write;
    for (auto scenario : {Failure::None, Failure::Open, Failure::Enumerate, Failure::Suspend,
             Failure::Context, Failure::Busy, Failure::Write}) {
        failure=scenario; enumerated=closed=suspended=resumed=writes=0; writeBeforeSafe=false;
        const bool result=WriteHotCodePatchSafely(reinterpret_cast<void*>(0x12340000), "1234567890123456", 16, api);
        const bool shouldWrite=scenario==Failure::None || scenario==Failure::Write;
        const bool passed=result==(scenario==Failure::None) && writes==(shouldWrite?1:0)
            && suspended==resumed && !writeBeforeSafe && closed>=(scenario==Failure::Open?2:3);
        if (!passed) {
            std::printf("hotpatch failure guard FAILED scenario=%d result=%d writes=%d suspend/resume=%d/%d closed=%d\n",
                static_cast<int>(scenario), result, writes, suspended, resumed, closed); return false;
        }
    }
    puts("hotpatch failure guards passed: open/enumeration/suspend/context/busy/write failure, no unsafe writes or stranded threads");
    return true;
}

bool PatternPages() {
    SYSTEM_INFO info{}; GetSystemInfo(&info);
    const auto page=info.dwPageSize;
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,page*4,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if (!image) return false;
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    dos->e_magic=IMAGE_DOS_SIGNATURE; dos->e_lfanew=0x80;
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(image+0x80);
    nt->Signature=IMAGE_NT_SIGNATURE;
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt->OptionalHeader.SizeOfImage=page*4;
    const unsigned char signature[]={0xA1,0xB2,0xC3,0xD4};
    std::memcpy(image+page-2,signature,2); // must not read across the inaccessible page
    std::memcpy(image+page*2+16,signature,sizeof(signature));
    std::memcpy(image+page*3-2,signature,sizeof(signature)); // readable region boundary
    DWORD previous{}; VirtualProtect(image+page,page,PAGE_NOACCESS,&previous);
    VirtualProtect(image+page*3,page,PAGE_READONLY,&previous);
    const auto module=reinterpret_cast<HMODULE>(image);
    const auto expected=reinterpret_cast<uintptr_t>(image+page*2+16);
    const auto single=FindPattern(module,"A1 B2 ? D4");
    const auto multiple=FindPatterns(module,"A1 B2 C3 D4",8);
    bool passed=single==expected && multiple.size()==2 && multiple[0]==expected
        && multiple[1]==reinterpret_cast<uintptr_t>(image+page*3-2)
        && FindPattern(module,"4")==0 && FindPattern(module,"GG")==0
        && FindPattern(module,nullptr)==0 && FindPatterns(module,"A1 B2 C3 D4",0).empty()
        && FindPattern(reinterpret_cast<HMODULE>(image+page),"AA")==0;
    std::uintptr_t output{}; std::size_t count{};
    const int rawSignature[]={0xA1,0xB2,0xC3,0xD4};
    // Models protection changing after VirtualQuery; the raw scan must reject
    // the result rather than propagate an access violation out to the host.
    passed &= !ScanReadablePatternRange(reinterpret_cast<uintptr_t>(image+page),
        reinterpret_cast<uintptr_t>(image+page*2), rawSignature, 4, &output, 1, count);
    VirtualProtect(image+page,page,PAGE_READWRITE|PAGE_GUARD,&previous);
    passed &= FindPattern(module,"A1 B2 ? D4")==expected;
    MEMORY_BASIC_INFORMATION guard{};
    VirtualQuery(image+page,&guard,sizeof(guard));
    passed &= (guard.Protect & PAGE_GUARD)!=0; // scanning must not consume the guard
    dos->e_lfanew = -0x80;
    passed &= FindPattern(module,"A1 B2 ? D4")==0;
    dos->e_lfanew = 0x80;
    VirtualFree(image,0,MEM_RELEASE);
    if (!passed) { puts("pattern page/malformed input guard FAILED"); return false; }
    puts("pattern scan passed: inaccessible/guard/header pages, protection races, adjacent readable regions, wildcard, malformed/null patterns and match limit");
    return true;
}

bool DiagnosticContention() {
    const auto before=PlayerMotionSampleCount();
    std::unique_lock suppressionLock(g_viewModelSuppressionMutex);
    std::promise<void> done; auto completion=done.get_future();
    std::thread worker([&] { RecordPlayerMotionFrame(nullptr,OverrideConfig{},EntityHandle{},nullptr,0,false); done.set_value(); });
    const bool nonblocking=completion.wait_for(std::chrono::milliseconds(250))==std::future_status::ready;
    suppressionLock.unlock(); worker.join();
    if (!nonblocking || PlayerMotionSampleCount()!=before) {
        puts("diagnostic suppression contention guard FAILED"); return false;
    }
    puts("diagnostic contention passed: contested suppression lock drops sample without blocking the callback");
    return true;
}
bool Run() { return PatchFailures() && PatternPages() && DiagnosticContention(); }

// Map the installed files as non-executable images; no LoadLibrary/DllMain,
// CS2 process or injection. This exercises the production scanner's VM-page
// handling against actual PE images as a separate optional compatibility run.
bool InstalledImagePatterns(const wchar_t* clientPath, const wchar_t* schemaPath) {
    struct Pattern { const char* name; const char* bytes; std::size_t expected; };
    const auto check = [](const wchar_t* path, std::initializer_list<Pattern> patterns) {
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE) return false;
        HANDLE mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);
        CloseHandle(file);
        if(!mapping) return false;
        auto* view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);
        CloseHandle(mapping);
        if(!view) return false;
        bool passed=true;
        for(const auto& pattern:patterns) {
            const auto found=FindPatterns(reinterpret_cast<HMODULE>(view),pattern.bytes);
            std::printf("mapped image pattern %s: %zu / %zu\n",pattern.name,found.size(),pattern.expected);
            passed &= found.size()==pattern.expected;
        }
        UnmapViewOfFile(view); return passed;
    };
    return check(clientPath,{{"entity-list",kPatternEntityList,1},{"entity-index",kPatternGetEntityFromIndex,1},
        {"split-screen",kPatternGetSplitScreenPlayer,2},{"attachments",kPatternAttachmentAccess,1},
        {"setup-view",kPatternSetupViewPatch,1}})
        && check(schemaPath,{{"schema-system",kPatternSchemaSystem,1},
            {"qualified-class",kPatternSchemaQualifiedClassLookup,1}});
}
}

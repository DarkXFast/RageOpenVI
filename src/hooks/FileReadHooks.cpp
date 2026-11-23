#include "main.h"
#include <unordered_map>
#include <shared_mutex>

using OpenBulkFn = HANDLE(*)(void*, const char*, __int64*);
using GetFileTimeFn = FILETIME(*)(void*, const char*);
using GetFileSizeFn = uint64_t(*)(void*, const char*);
using GetAttrsFn = uint64_t(*)(void*, const char*);

inline OpenBulkFn g_origOpenBulk = nullptr;
inline GetFileTimeFn g_origGetFileTime = nullptr;
inline GetFileSizeFn g_origGetFileSize = nullptr;
inline GetAttrsFn g_origGetAttribs = nullptr;

static const uint8_t EXISTS = 1;
static const uint8_t NOT_EXISTS = 2;
static std::unordered_map<std::string, uint8_t> g_modsCache;
static std::shared_mutex g_modsCacheMutex;

static bool TryModsPath(const char* path, std::string& outModsPath)
{
    outModsPath = std::string("mods/") + path;

    // Used a hashmap for lookups so that we don't call GetFileAttributeA every time
    {
        std::shared_lock lock(g_modsCacheMutex);
        auto it = g_modsCache.find(outModsPath);
        if (it != g_modsCache.end())
            return it->second == EXISTS;
    }

    bool exists = GetFileAttributesA(outModsPath.c_str()) != INVALID_FILE_ATTRIBUTES;
    {
        std::unique_lock lock(g_modsCacheMutex);
        g_modsCache[outModsPath] = exists ? EXISTS : NOT_EXISTS;
    }
    return exists;
}

static HANDLE OpenBulkHook(void* device, const char* path, __int64* a3)
{
    logger::log("info", "[%s] Called with path /%s", __FUNCTION__, path);

    std::string modsPath;
    if (path && TryModsPath(path, modsPath))
    {
        logger::log("mods", "[%s] Redirecting to /%s", __FUNCTION__, modsPath.c_str());
        HANDLE result = g_origOpenBulk(device, modsPath.c_str(), a3);
        if (result && result != INVALID_HANDLE_VALUE)
            return result;
    }
    return g_origOpenBulk(device, path, a3);
}

static FILETIME GetFileTimeHook(void* device, const char* path)
{
    logger::log("info", "[%s] Called with path /%s", __FUNCTION__, path);

    std::string modsPath;
    if (path && TryModsPath(path, modsPath))
    {
        logger::log("mods", "[%s] Redirecting to /%s", __FUNCTION__, modsPath.c_str());
        return g_origGetFileTime(device, modsPath.c_str());
    }
    return g_origGetFileTime(device, path);
}

static uint64_t GetFileSizeHook(void* device, const char* path)
{
    logger::log("info", "[%s] Called with path /%s", __FUNCTION__, path);

    std::string modsPath;
    if (path && TryModsPath(path, modsPath))
    {
        logger::log("mods", "[%s] Redirecting to /%s", __FUNCTION__, modsPath.c_str());
        return g_origGetFileSize(device, modsPath.c_str());
    }
    return g_origGetFileSize(device, path);
}

static uint64_t GetAttribsHook(void* device, const char* path)
{
    logger::log("info", "[%s] Called with path /%s", __FUNCTION__, path);

    std::string modsPath;
    if (path && TryModsPath(path, modsPath))
    {
        logger::log("mods", "[%s] Redirecting to /%s", __FUNCTION__, modsPath.c_str());
        return g_origGetAttribs(device, modsPath.c_str());
    }
    return g_origGetAttribs(device, path);
}

static memory::InitFuncs FileReadHooks([] {

    if (auto fn = patterns::find(patterns::OpenBulk))
        memory(fn).hook(OpenBulkHook, &g_origOpenBulk);

    // GetFileTime and GetFileSize differ only in the last byte of their patterns (stat field offset)
    if (auto fn = patterns::find(patterns::GetFileTime))
        memory(fn).hook(GetFileTimeHook, &g_origGetFileTime);

    if (auto fn = patterns::find(patterns::GetFileSize))
        memory(fn).hook(GetFileSizeHook, &g_origGetFileSize);

    if (auto fn = patterns::find(patterns::GetAttributes))
        memory(fn).hook(GetAttribsHook, &g_origGetAttribs);
});

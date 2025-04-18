#include <StdInc.h>
#include <Hooking.h>
#include <Hooking.Stubs.h>
#include <Streaming.h>

static void* g_storeMgr;

static hook::cdecl_stub<void(bool)> g_loadObjectsNow([]()
{
	return hook::get_call(hook::get_pattern("41 B8 14 00 00 00 03 D3 E8", 0xF));
});

static hook::cdecl_stub<void(void*, uint32_t* fileIdx, const char* registerName, uint32_t handle, uint32_t collectionId, void* unk, bool unk2, void* unk3, bool unk4)> g_registerObject([]()
{
	return hook::get_call<void*>(hook::get_pattern<char>("B2 01 48 8B CD 45 8A E0 4D 0F 45 F9 E8", -0x25) + 0xE9);
});

static hook::cdecl_stub<void(void*, uint32_t, int)> g_requestObject([]()
{
	return hook::get_call(hook::get_pattern("41 B8 14 00 00 00 03 D3 E8", 8));
});

static hook::cdecl_stub<bool(void*, uint32_t)> g_releaseObject([]()
{
	return hook::get_call(hook::get_pattern("45 33 C0 03 D7 E8 ? ? ? ? 48 8B 03 48 8B CB", 5));
});

static hook::cdecl_stub<bool(void*, uint32_t, int)> g_releaseSystemObject([]()
{
	return hook::get_pattern("8B CA 4D 8B 11 45 0F B7 5C CA 06 45", -0xB);
});

static hook::cdecl_stub<bool(void*, uint32_t)> g_unregisterObject([]()
{
	return hook::get_pattern("8B C2 48 8B 11 F6 44 C2");
});

static hook::cdecl_stub<streaming::strStreamingModule**(void*, uint32_t)> g_getStreamingModule([]()
{
	return hook::get_pattern("45 33 C0 41 FF C9 41 8B C1 D1 F8 48", -0xD);
});

static hook::cdecl_stub<streaming::strStreamingModule*(void*, const char*)> g_getStreamingModuleFromExt([]()
{
	return hook::get_call(hook::get_pattern("74 15 48 8D 50 01 48 8D", 13));
});

static hook::cdecl_stub<uint32_t*(uint32_t*, const char*, bool, const char*, bool)> g_registerRawStreamingFile([]()
{
	return hook::get_pattern("B2 01 48 8B CD 45 8A E0 4D 0F 45 F9 E8", -0x25);
});

static hook::cdecl_stub<size_t(StreamingDataEntry*, uint32_t, void*, bool)> _computeVirtualSize([]()
{
	return hook::get_call(hook::get_pattern("FF 46 0C 8B 57 04", 15));
});

static hook::cdecl_stub<size_t(StreamingDataEntry*, uint32_t)> _computePhysicalSize([]()
{
	return hook::get_call(hook::get_pattern("FF 46 0C 8B 57 04", 31));
});

static hook::cdecl_stub<bool(streaming::Manager*, uint32_t, int)> _isReadyToDelete([]()
{
	return hook::get_pattern("89 54 24 10 48 83 EC 28 48 8B 01 41 81 E0 0E FF");
});

static hook::cdecl_stub<void(streaming::Manager*, atArray<uint32_t>&, uint32_t)> _getDependents([]()
{
	return hook::get_pattern("8B F8 48 8B EA 4C 8B F1 85 F6 74 2B 8B D3", -32);
});

static hook::cdecl_stub<void(uint32_t, atArray<uint32_t>&, uint32_t)> _getDependentsInner([]()
{
	return (void*)hook::get_call(hook::get_pattern<char>("8B F8 48 8B EA 4C 8B F1 85 F6 74 2B 8B D3", -32) + 0x42);
});

size_t StreamingDataEntry::ComputePhysicalSize(uint32_t strIndex)
{
	return _computePhysicalSize(this, strIndex);
}

size_t StreamingDataEntry::ComputeVirtualSize(uint32_t strIndex, void* a3, bool a4)
{
	return _computeVirtualSize(this, strIndex, a3, a4);
}

static rage::strStreamingAllocator* g_streamingAllocator;

namespace rage
{
	strStreamingAllocator* strStreamingAllocator::GetInstance()
	{
		return g_streamingAllocator;
	}
}

rage::fiCollection** g_fiCollections;
static void* (*g_orig_rage_pgStreamer_ctor)(void*);
int currentIdx = 1;

void AddRawStreamer()
{
	auto alloc8ed = rage::GetAllocator()->Allocate(2048, 16, 0);

	auto swap = g_fiCollections[0];
	g_orig_rage_pgStreamer_ctor(alloc8ed);
	g_fiCollections[0] = swap;

	g_fiCollections[currentIdx++] = static_cast<rage::fiCollection*>(alloc8ed);
}

void* rage_pgStreamer_ctor(void* self)
{
	// the game's own raw streamer
	auto ret = g_orig_rage_pgStreamer_ctor(self);

	// for fivem
	AddRawStreamer();

	// for mod loader
	AddRawStreamer();

	return ret;
}

namespace streaming
{
	bool Manager::IsObjectReadyToDelete(uint32_t streamingIndex, int flags)
	{
		return _isReadyToDelete(this, streamingIndex, flags);
	}

	void Manager::FindAllDependents(atArray<uint32_t>& outIndices, uint32_t objectId)
	{
		return _getDependents(this, outIndices, objectId);
	}

	void Manager::FindDependentsInner(uint32_t selfId, atArray<uint32_t>& outIndices, uint32_t objectId)
	{
		// this would call the original, except it will return a physical in-image index and not necessarily the real object
		//_getDependentsInner(selfId, outIndices, objectId);

		if (selfId != -1)
		{
			uint32_t outDeps[50];
			std::uninitialized_fill(outDeps, &outDeps[50], -1);

			auto module = moduleMgr.GetStreamingModule(selfId);
			int numDeps = module->GetDependencies(selfId - module->baseIdx, outDeps, std::size(outDeps));

			for (int i = 0; i < numDeps; i++)
			{
				if (outDeps[i] == objectId)
				{
					outIndices.Set(outIndices.GetCount(), selfId);
					break;
				}
			}
		}
	}

	void LoadObjectsNow(bool a1)
	{
		g_loadObjectsNow(a1);
	}

	void Manager::RegisterObject(uint32_t* fileIdx, const char* registerName, uint32_t handle, uint32_t collectionId, void* unk, bool unk2, void* unk3, bool unk4)
	{
		return g_registerObject(this, fileIdx, registerName, handle, collectionId, unk, unk2, unk3, unk4);
	}

	void Manager::RequestObject(uint32_t objectId, int flags)
	{
		g_requestObject(this, objectId, flags);
	}

	bool Manager::ReleaseObject(uint32_t objectId)
	{
		return g_releaseObject(this, objectId);
	}

	bool Manager::ReleaseObject(uint32_t objectId, int flags)
	{
		return g_releaseSystemObject(this, objectId, flags);
	}

	bool Manager::UnregisterObject(uint32_t objectId)
	{
		return g_unregisterObject(this, objectId);
	}

	strStreamingModule* strStreamingModuleMgr::GetStreamingModule(int index)
	{
		return *g_getStreamingModule(this, index);
	}

	strStreamingModule* strStreamingModuleMgr::GetStreamingModule(const char* extension)
	{
		return g_getStreamingModuleFromExt(this, extension);
	}

	Manager* Manager::GetInstance()
	{
		return (Manager*)g_storeMgr;
	}

	uint32_t* RegisterRawStreamingFile(uint32_t* fileId, const char* fileName, bool unkTrue, const char* registerAs, bool errorIfFailed)
	{
		rage::fiCollection* rawStreamer = nullptr;

		if (auto collectionIdx = GetRawStreamerForFile(fileName, &rawStreamer))
		{
			auto fileIdx = rawStreamer->GetEntryByName(fileName);
			if (fileIdx != uint16_t(-1))
			{
				const char* registerName = fileName;

				if (registerAs)
				{
					registerName = registerAs;
				}

				uint8_t unkVal;
				Manager::GetInstance()->RegisterObject(fileId, registerName, fileIdx | collectionIdx << 16, collectionIdx, &unkVal, unkTrue, nullptr, false);
			}

			return fileId;
		}

		return g_registerRawStreamingFile(fileId, fileName, unkTrue, registerAs, errorIfFailed);
	}

	int GetCollectionIndexForFileName(const char* fileName)
	{
		// anything related to mods installed by the users themselves
		if (strncmp(fileName, "faux_pack", 9) == 0 || strncmp(fileName, "addons:/", 8) == 0)
		{
			return 2;
		}
		// for now, only texture dictionaries have a custom streamer as its the most used
		auto len = strlen(fileName);
		if (len > 4 && strcmp(fileName + len - 3, "ytd") == 0)
		{
			return 1;
		}

		return 0;
	}

	STREAMING_EXPORT int GetRawStreamerForFile(const char* fileName, rage::fiCollection** collection)
	{
		auto idx = GetCollectionIndexForFileName(fileName);
		*collection = g_fiCollections[idx];
		return idx;
	}

	STREAMING_EXPORT rage::fiCollection* GetRawStreamerByIndex(uint16_t idx)
	{
		return g_fiCollections[idx];
	}
}

static HookFunction hookFunction([] ()
{
	g_storeMgr = hook::get_address<void*>(hook::get_pattern<char>("74 1A 8B 15 ? ? ? ? 48 8D 0D ? ? ? ? 41", 11));
	g_streamingAllocator = hook::get_address<decltype(g_streamingAllocator)>(hook::get_pattern("44 8B 46 04 48 8D 0D ? ? ? ? 49 8B D2 44", 7));

	g_orig_rage_pgStreamer_ctor = hook::trampoline(hook::get_pattern("48 8B CB 33 D2 41 B8 00 02 00 00 E8", -0x29), rage_pgStreamer_ctor);

	// the first 3 indexes are for rawstreamers, rest is for packfiles
	auto addr = hook::get_pattern<char>("41 0F B7 D2 4C 8D");
	hook::put<uint32_t>(addr, 0x02528D41); // lea    edx,[r10+0x2]

	g_fiCollections = hook::get_address<rage::fiCollection**>(addr + 7);
});

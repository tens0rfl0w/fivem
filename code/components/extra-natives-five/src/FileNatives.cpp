#include <StdInc.h>

#include <boost/algorithm/string.hpp>
#include <botan/sha2_32.h>
#include <concurrent_queue.h>
#include <concurrent_unordered_map.h>

#include "fiDevice.h"
#include "ICoreGameInit.h"
#include "ScriptEngine.h"

static constexpr std::array<std::string_view, 7> kWhitelistedPaths = {
	"platform:/",
	"common:/",
	"update:/",
	"update2:/",
	"audio:/",
	"commoncrc:/",
	"platformcrc:/"
};

namespace
{
	concurrency::concurrent_queue<std::string> g_fetchQueue;
	concurrency::concurrent_unordered_map<std::string, std::string> g_fileHashes;
	std::once_flag g_workerCreated;
	std::condition_variable g_workAvailableCv;

	bool IsPathWhitelisted(const std::string_view& path)
	{
		return std::any_of(kWhitelistedPaths.begin(), kWhitelistedPaths.end(),
		[&path](const auto& prefix)
		{
			return boost::algorithm::starts_with(path, prefix);
		});
	}

	std::string ToHexString(const uint8_t* data, const size_t length)
	{
		static constexpr char HEX_CHARS[] = "0123456789ABCDEF";
		std::string result(length * 2, '\0');

		for (size_t i = 0; i < length; ++i)
		{
			result[2 * i] = HEX_CHARS[data[i] >> 4];
			result[2 * i + 1] = HEX_CHARS[data[i] & 0xF];
		}

		return result;
	}

	void CalculateFileHash(const std::string& path)
	{
		std::string result = "missing";
		rage::fiDevice* device = rage::fiDevice::GetDevice(path.c_str(), true);

		if (device)
		{
			const auto handle = device->Open(path.c_str(), true);

			if (handle != -1)
			{
				const size_t fileLength = device->GetFileLength(handle);
				constexpr size_t CHUNK_SIZE = 65536; // 64 KB
				std::vector<char> buffer(CHUNK_SIZE);

				Botan::SHA_256 hashFunction;

				size_t bytesRead = 0;
				while (bytesRead < fileLength) {
					const size_t toRead = std::min(CHUNK_SIZE, fileLength - bytesRead);
					if (device->Read(handle, buffer.data(), toRead) > 0)
					{
						hashFunction.update(reinterpret_cast<const uint8_t*>(buffer.data()), toRead);
						bytesRead += toRead;
					}
					else
					{
						break;
					}
				}

				device->Close(handle);

				std::vector<uint8_t> hash(hashFunction.output_length());
				hashFunction.final(hash.data());

				result = ToHexString(hash.data(), hash.size());
			}
		}

		g_fileHashes[path] = result;
	}

	void FetchWorker()
	{
		SetThreadName(-1, "[Cfx] File Hasher Thread");

		std::mutex mtx;
		std::string path;
		while (true)
		{
			std::unique_lock lock(mtx);
			g_workAvailableCv.wait(lock);

			// Race condition here

			while (g_fetchQueue.try_pop(path))
			{
				CalculateFileHash(path);
				// Yield?
			}
		}
	}
}

static HookFunction hookFunction([]() {
	fx::ScriptEngine::RegisterNativeHandler("FETCH_MOUNTED_FILE_HASH", [](fx::ScriptContext& context)
	{
		const std::string path = context.CheckArgument<const char*>(0);

		if (!IsPathWhitelisted(path))
		{
			context.SetResult(false);
			return;
		}

		g_fileHashes[path] = "";
		g_fetchQueue.push(std::move(path));
		g_workAvailableCv.notify_one();

		context.SetResult(true);
	});

	fx::ScriptEngine::RegisterNativeHandler("GET_MOUNTED_FILE_HASH", [](fx::ScriptContext& context)
	{
		const std::string path = context.CheckArgument<const char*>(0);
		const auto it = g_fileHashes.find(path);

		if (it != g_fileHashes.end())
		{
			context.SetResult(it->second.c_str());
		}
		else
		{
			context.SetResult<const char*>("missing");
		}
	});

	fx::ScriptEngine::RegisterNativeHandler("IS_MOUNTED_FILE_HASH_READY", [](fx::ScriptContext& context)
	{
		const std::string path = context.CheckArgument<const char*>(0);
		const auto it = g_fileHashes.find(path);

		context.SetResult(it != g_fileHashes.end() && it->second != "");
	});

	Instance<ICoreGameInit>::Get()->OnGameFinalizeLoad.Connect([]()
	{
		std::call_once(g_workerCreated, []()
		{
			std::thread(FetchWorker).detach();
		});
	});

	Instance<ICoreGameInit>::Get()->OnShutdownSession.Connect([]()
	{
		std::string discard;
		while (g_fetchQueue.try_pop(discard)) {}

		//g_fileHashes.clear();
	});
});

// Init thread correctly
// Markdown
// Rpf files
// Additional paths / test paths
// Clean hashes on shutdown
// Remove after read

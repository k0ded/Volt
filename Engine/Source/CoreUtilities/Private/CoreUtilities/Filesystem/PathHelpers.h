#pragma once

#include <CoreUtilities/String/StringView.h>

namespace Helpers
{
	struct PathParts
	{
		WStringView full;

		WStringView parent;
		WStringView filename;
		WStringView stem;
		WStringView extension;
	};

	struct RootParts
	{
		WStringView rootName;
		WStringView rootDirectory;
		size_t pathStart = 0;
	};

	static PathParts ParsePath(WStringView path)
	{
		PathParts result;
		result.full = path;

		const wchar_t* first = path.data();
		const wchar_t* last = first + path.size();

		// Find filename
		const wchar_t* filename = last;

		for (const wchar_t* p = last; p != first;)
		{
			--p;
			if (*p == L'/' || *p == L'\\')
			{
				filename = p + 1;
				result.parent = WStringView(first, static_cast<size_t>(p - first));
				break;
			}

			filename = p;
		}

		if (filename == first)
		{
			result.parent = WStringView(first, 0);
		}

		// Find extension
		const wchar_t* ext = last;

		for (const wchar_t* p = last; p != filename;)
		{
			--p;

			if (*p == L'.')
			{
				ext = p;
				break;
			}

			if (*p == L'/' || *p == L'\\')
			{
				break;
			}
		}

		result.filename = WStringView(filename, static_cast<size_t>(last - filename));

		if (ext != last)
		{
			result.stem = WStringView(filename, static_cast<size_t>(ext - filename));
			result.extension = WStringView(ext, static_cast<size_t>(last - ext));
		}
		else
		{
			result.stem = result.filename;
			result.extension = WStringView(last, 0);
		}

		return result;
	}

	static RootParts ParseRoot(WStringView path)
	{
		RootParts result;

		const wchar_t* s = path.data();
		size_t n = path.size();

		if (n >= 2 && s[1] == L':')
		{
			// Drive root
			result.rootName = WStringView(s, 2);

			if (n >= 3 && (s[2] == L'\\' || s[2] == L'/'))
			{
				result.rootDirectory = WStringView(s + 2, 1);
				result.pathStart = 3;
			}
			else
			{
				result.pathStart = 2;
			}

			return result;
		}

		if (n >= 2 && s[0] == L'\\' && s[1] == L'\\')
		{
			// UNC root
			const wchar_t* p = s + 2;

			const wchar_t* serverEnd = p;
			while (serverEnd < s + n && *serverEnd != L'\\' && *serverEnd != L'/')
			{
				++serverEnd;
			}

			const wchar_t* shareEnd = serverEnd;
			if (shareEnd < s + n)
			{
				++shareEnd;
				while (shareEnd < s + n && *shareEnd != L'\\' && *shareEnd != L'/')
				{
					++shareEnd;
				}
			}

			result.rootName = WStringView(s, shareEnd - s);

			if (shareEnd < s + n)
			{
				result.rootDirectory = WStringView(shareEnd, 1);
				result.pathStart = (shareEnd - s) + 1;
			}
			else
			{
				result.pathStart = shareEnd - s;
			}

			return result;
		}

		if (n >= 1 && (s[0] == L'\\' || s[0] == L'/'))
		{
			result.rootDirectory = WStringView(s, 1);
			result.pathStart = 1;
		}

		return result;
	}
}

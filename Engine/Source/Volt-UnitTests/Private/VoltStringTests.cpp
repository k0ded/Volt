#include <CoreUtilities/VoltString.h>

#include <gtest/gtest.h>

namespace UnitTests
{
	// Default constructor
	TEST(VoltString, DefaultConstructor)
	{
		String s;
		ASSERT_EQ(s.size(), 0);
		ASSERT_TRUE(s.empty());
		ASSERT_STREQ(s.c_str(), "");
	}

	// Construct from const char*
	TEST(VoltString, ConstructFromCStr)
	{
		String s("hello");
		ASSERT_EQ(s.size(), 5);
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// Construct from const char* with length
	TEST(VoltString, ConstructFromCStrWithLength)
	{
		String s("hello world", 5);
		ASSERT_EQ(s.size(), 5);
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// Construct from repeated char
	TEST(VoltString, ConstructRepeatedChar)
	{
		String s(5, 'a');
		ASSERT_EQ(s.size(), 5);
		ASSERT_STREQ(s.c_str(), "aaaaa");
	}

	// Copy constructor
	TEST(VoltString, CopyConstructor)
	{
		String original("hello");
		String copy(original);
		ASSERT_STREQ(copy.c_str(), "hello");
		ASSERT_EQ(copy.size(), original.size());
		ASSERT_NE(copy.data(), original.data());
	}

	// Move constructor
	TEST(VoltString, MoveConstructor)
	{
		String original("hello");
		String moved(std::move(original));

		ASSERT_STREQ(moved.c_str(), "hello");
		ASSERT_TRUE(original.empty());
	}

	// Substring constructor
	TEST(VoltString, SubstringConstructor)
	{
		String original("hello world");
		String sub(original, 6, 5);
		ASSERT_STREQ(sub.c_str(), "world");
	}

	// Initializer list constructor
	TEST(VoltString, InitializerListConstructor)
	{
		String s({'h', 'e', 'l', 'l', 'o'});
		ASSERT_EQ(s.size(), 5);
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// Range constructor
	TEST(VoltString, RangeConstructor)
	{
		const char* str = "hello world";
		String s(str, str + 5);
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// operator= from const char*
	TEST(VoltString, AssignFromCStr)
	{
		String s;
		s = "hello";
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// operator= from char
	TEST(VoltString, AssignFromChar)
	{
		String s;
		s = 'x';
		ASSERT_EQ(s.size(), 1);
		ASSERT_STREQ(s.c_str(), "x");
	}

	// Copy assignment
	TEST(VoltString, CopyAssignment)
	{
		String original("hello");
		String s;
		s = original;
		ASSERT_STREQ(s.c_str(), "hello");
		ASSERT_NE(s.data(), original.data());
	}

	// Move assignment
	TEST(VoltString, MoveAssignment)
	{
		String original("hello");
		String s;
		s = std::move(original);
		ASSERT_STREQ(s.c_str(), "hello");
		ASSERT_TRUE(original.empty());
	}

	// size, length, empty
	TEST(VoltString, SizeLengthEmpty)
	{
		String empty;
		ASSERT_EQ(empty.size(), 0);
		ASSERT_EQ(empty.length(), 0);
		ASSERT_TRUE(empty.empty());

		String s("abc");
		ASSERT_EQ(s.size(), 3);
		ASSERT_EQ(s.length(), 3);
		ASSERT_FALSE(s.empty());
	}

	// capacity and reserve
	TEST(VoltString, CapacityAndReserve)
	{
		String s;
		s.reserve(100);
		ASSERT_GE(s.capacity(), 100);
		ASSERT_EQ(s.size(), 0);
	}

	// c_str and data
	TEST(VoltString, CStrAndData)
	{
		String s("hello");
		ASSERT_STREQ(s.c_str(), "hello");
		ASSERT_STREQ(s.data(), "hello");
		ASSERT_EQ(s.c_str(), s.data());
	}

	// operator[]
	TEST(VoltString, OperatorIndex)
	{
		String s("hello");
		ASSERT_EQ(s[0], 'h');
		ASSERT_EQ(s[1], 'e');
		ASSERT_EQ(s[4], 'o');

		s[0] = 'H';
		ASSERT_EQ(s[0], 'H');
	}

	// at
	TEST(VoltString, At)
	{
		String s("hello");
		ASSERT_EQ(s.at(0), 'h');
		ASSERT_EQ(s.at(4), 'o');
	}

	// front and back
	TEST(VoltString, FrontAndBack)
	{
		String s("hello");
		ASSERT_EQ(s.front(), 'h');
		ASSERT_EQ(s.back(), 'o');
	}

	// append string
	TEST(VoltString, AppendString)
	{
		String s("hello");
		String other(" world");
		s.append(other);
		ASSERT_STREQ(s.c_str(), "hello world");
	}

	// append const char*
	TEST(VoltString, AppendCStr)
	{
		String s("hello");
		s.append(" world");
		ASSERT_STREQ(s.c_str(), "hello world");
	}

	// append repeated char
	TEST(VoltString, AppendRepeatedChar)
	{
		String s("hello");
		s.append(3, '!');
		ASSERT_STREQ(s.c_str(), "hello!!!");
	}

	// operator+=
	TEST(VoltString, OperatorPlusEquals)
	{
		String s("hello");
		s += " world";
		ASSERT_STREQ(s.c_str(), "hello world");

		s += '!';
		ASSERT_STREQ(s.c_str(), "hello world!");

		String other(" bye");
		s += other;
		ASSERT_STREQ(s.c_str(), "hello world! bye");
	}

	// push_back and pop_back
	TEST(VoltString, PushBackPopBack)
	{
		String s("hell");
		s.push_back('o');
		ASSERT_STREQ(s.c_str(), "hello");

		s.pop_back();
		ASSERT_STREQ(s.c_str(), "hell");
		ASSERT_EQ(s.size(), 4);
	}

	// insert at position
	TEST(VoltString, InsertAtPosition)
	{
		String s("helo");
		s.insert(2, "l");
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// insert string at position
	TEST(VoltString, InsertStringAtPosition)
	{
		String s("hd");
		String mid("ello worl");
		s.insert(1, mid);
		ASSERT_STREQ(s.c_str(), "hello world");
	}

	// insert repeated char at position
	TEST(VoltString, InsertRepeatedCharAtPosition)
	{
		String s("heo");
		s.insert(2, 2, 'l');
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// erase by position
	TEST(VoltString, EraseByPosition)
	{
		String s("hello world");
		s.erase(5, 6);
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// erase single character by iterator
	TEST(VoltString, EraseByIterator)
	{
		String s("hello");
		s.erase(s.begin() + 1);
		ASSERT_STREQ(s.c_str(), "hllo");
	}

	// erase range by iterator
	TEST(VoltString, EraseRangeByIterator)
	{
		String s("hello world");
		s.erase(s.begin() + 5, s.end());
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// clear
	TEST(VoltString, Clear)
	{
		String s("hello");
		s.clear();
		ASSERT_TRUE(s.empty());
		ASSERT_EQ(s.size(), 0);
		ASSERT_STREQ(s.c_str(), "");
	}

	// replace
	TEST(VoltString, ReplaceByPosition)
	{
		String s("hello world");
		s.replace(6, 5, "there");
		ASSERT_STREQ(s.c_str(), "hello there");
	}

	// replace with different length
	TEST(VoltString, ReplaceWithDifferentLength)
	{
		String s("hello world");
		s.replace(5, 6, "!");
		ASSERT_STREQ(s.c_str(), "hello!");
	}

	// find
	TEST(VoltString, Find)
	{
		String s("hello world hello");
		ASSERT_EQ(s.find("world"), 6);
		ASSERT_EQ(s.find("hello"), 0);
		ASSERT_EQ(s.find("hello", 1), 12);
		ASSERT_EQ(s.find("xyz"), String::npos);
		ASSERT_EQ(s.find('w'), 6);
	}

	// rfind
	TEST(VoltString, Rfind)
	{
		String s("hello world hello");
		ASSERT_EQ(s.rfind("hello"), 12);
		ASSERT_EQ(s.rfind("world"), 6);
		ASSERT_EQ(s.rfind("xyz"), String::npos);
		ASSERT_EQ(s.rfind('o'), 16);
	}

	// find_first_of
	TEST(VoltString, FindFirstOf)
	{
		String s("hello world");
		ASSERT_EQ(s.find_first_of("aeiou"), 1); // 'e'
		ASSERT_EQ(s.find_first_of("xyz"), String::npos);
	}

	// find_last_of
	TEST(VoltString, FindLastOf)
	{
		String s("hello world");
		ASSERT_EQ(s.find_last_of("aeiou"), 7); // 'o' in "world"
		ASSERT_EQ(s.find_last_of("xyz"), String::npos);
	}

	// find_first_not_of
	TEST(VoltString, FindFirstNotOf)
	{
		String s("aaabcd");
		ASSERT_EQ(s.find_first_not_of('a'), 3);
		ASSERT_EQ(s.find_first_not_of("abc"), 5); // 'd'
	}

	// find_last_not_of
	TEST(VoltString, FindLastNotOf)
	{
		String s("abcddd");
		ASSERT_EQ(s.find_last_not_of('d'), 2);
		ASSERT_EQ(s.find_last_not_of("cd"), 1); // 'b'
	}

	// substr
	TEST(VoltString, Substr)
	{
		String s("hello world");
		String sub = s.substr(6);
		ASSERT_STREQ(sub.c_str(), "world");

		String sub2 = s.substr(0, 5);
		ASSERT_STREQ(sub2.c_str(), "hello");
	}

	// compare
	TEST(VoltString, Compare)
	{
		String a("abc");
		String b("abc");
		String c("abd");
		String d("ab");

		ASSERT_EQ(a.compare(b), 0);
		ASSERT_LT(a.compare(c), 0);
		ASSERT_GT(c.compare(a), 0);
		ASSERT_GT(a.compare(d), 0);
		ASSERT_EQ(a.compare("abc"), 0);
	}

	// comparei (case insensitive)
	TEST(VoltString, Comparei)
	{
		String a("Hello");
		String b("hello");
		String c("HELLO");

		ASSERT_EQ(a.comparei(b), 0);
		ASSERT_EQ(a.comparei(c), 0);
		ASSERT_EQ(a.comparei("hElLo"), 0);
	}

	// make_lower
	TEST(VoltString, MakeLower)
	{
		String s("Hello World");
		s.make_lower();
		ASSERT_STREQ(s.c_str(), "hello world");
	}

	// make_upper
	TEST(VoltString, MakeUpper)
	{
		String s("Hello World");
		s.make_upper();
		ASSERT_STREQ(s.c_str(), "HELLO WORLD");
	}

	// ltrim
	TEST(VoltString, Ltrim)
	{
		String s("   hello   ");
		s.ltrim();
		ASSERT_STREQ(s.c_str(), "hello   ");
	}

	// rtrim
	TEST(VoltString, Rtrim)
	{
		String s("   hello   ");
		s.rtrim();
		ASSERT_STREQ(s.c_str(), "   hello");
	}

	// trim
	TEST(VoltString, Trim)
	{
		String s("   hello   ");
		s.trim();
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// ltrim with custom characters
	TEST(VoltString, LtrimCustom)
	{
		String s("xxxhello");
		s.ltrim("x");
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// rtrim with custom characters
	TEST(VoltString, RtrimCustom)
	{
		String s("helloyyy");
		s.rtrim("y");
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// trim with custom characters
	TEST(VoltString, TrimCustom)
	{
		String s("**hello**");
		s.trim("*");
		ASSERT_STREQ(s.c_str(), "hello");
	}

	// left
	TEST(VoltString, Left)
	{
		String s("hello world");
		String l = s.left(5);
		ASSERT_STREQ(l.c_str(), "hello");
	}

	// right
	TEST(VoltString, Right)
	{
		String s("hello world");
		String r = s.right(5);
		ASSERT_STREQ(r.c_str(), "world");
	}

	// operator==
	TEST(VoltString, OperatorEqual)
	{
		String a("hello");
		String b("hello");
		String c("world");

		ASSERT_TRUE(a == b);
		ASSERT_FALSE(a == c);
		ASSERT_TRUE(a == "hello");
		ASSERT_TRUE("hello" == a);
		ASSERT_FALSE(a == "world");
	}

	// operator+ concatenation
	TEST(VoltString, OperatorPlus)
	{
		String a("hello");
		String b(" world");
		String c = a + b;
		ASSERT_STREQ(c.c_str(), "hello world");

		String d = a + " there";
		ASSERT_STREQ(d.c_str(), "hello there");

		String e = "say " + a;
		ASSERT_STREQ(e.c_str(), "say hello");

		String f = a + '!';
		ASSERT_STREQ(f.c_str(), "hello!");
	}

	// operator+ with rvalues
	TEST(VoltString, OperatorPlusRvalue)
	{
		String result = String("hello") + String(" world");
		ASSERT_STREQ(result.c_str(), "hello world");

		String result2 = String("hello") + " world";
		ASSERT_STREQ(result2.c_str(), "hello world");
	}

	// swap
	TEST(VoltString, Swap)
	{
		String a("hello");
		String b("world");
		a.swap(b);
		ASSERT_STREQ(a.c_str(), "world");
		ASSERT_STREQ(b.c_str(), "hello");
	}

	// resize
	TEST(VoltString, Resize)
	{
		String s("hello");
		s.resize(3);
		ASSERT_EQ(s.size(), 3);
		ASSERT_STREQ(s.c_str(), "hel");

		s.resize(5, 'x');
		ASSERT_EQ(s.size(), 5);
		ASSERT_STREQ(s.c_str(), "helxx");
	}

	// shrink_to_fit
	TEST(VoltString, ShrinkToFit)
	{
		String s;
		s.reserve(100);
		s = "hi";
		ASSERT_GE(s.capacity(), 100);
		s.shrink_to_fit();
		ASSERT_LE(s.capacity(), s.size() + 23); // SSO capacity or close to size
	}

	// Iterators
	TEST(VoltString, Iterators)
	{
		String s("hello");

		// Forward iteration
		String::iterator it = s.begin();
		ASSERT_EQ(*it, 'h');
		++it;
		ASSERT_EQ(*it, 'e');

		ASSERT_EQ(s.end() - s.begin(), 5);

		// Const iteration
		const String& cs = s;
		ASSERT_EQ(*cs.begin(), 'h');
		ASSERT_EQ(*cs.cbegin(), 'h');
		ASSERT_EQ(cs.cend() - cs.cbegin(), 5);
	}

	// Reverse iterators
	TEST(VoltString, ReverseIterators)
	{
		String s("hello");
		ASSERT_EQ(*s.rbegin(), 'o');
		String reversed;
		for (auto it = s.rbegin(); it != s.rend(); ++it)
		{
			reversed.push_back(*it);
		}
		ASSERT_STREQ(reversed.c_str(), "olleh");
	}

	// SSO - small strings stay in local buffer
	TEST(VoltString, SSOSmallString)
	{
		// SSO capacity for char is 23 bytes on 64-bit
		String s("short");
		ASSERT_TRUE(s.validate());
		ASSERT_EQ(s.size(), 5);
		ASSERT_STREQ(s.c_str(), "short");
	}

	// Heap allocation - large strings go to heap
	TEST(VoltString, HeapLargeString)
	{
		// Create a string longer than SSO capacity (23 chars for char on 64-bit)
		String s("this is a string that is definitely longer than the SSO buffer capacity");
		ASSERT_TRUE(s.validate());
		ASSERT_STREQ(s.c_str(), "this is a string that is definitely longer than the SSO buffer capacity");
	}

	// assign operations
	TEST(VoltString, AssignString)
	{
		String s;
		String other("hello");
		s.assign(other);
		ASSERT_STREQ(s.c_str(), "hello");
	}

	TEST(VoltString, AssignCStrWithLength)
	{
		String s;
		s.assign("hello world", 5);
		ASSERT_STREQ(s.c_str(), "hello");
	}

	TEST(VoltString, AssignRepeatedChar)
	{
		String s;
		s.assign(5, 'x');
		ASSERT_STREQ(s.c_str(), "xxxxx");
	}

	TEST(VoltString, AssignMoveString)
	{
		String original("hello");
		String s;
		s.assign(std::move(original));
		ASSERT_STREQ(s.c_str(), "hello");
		ASSERT_TRUE(original.empty());
	}

	// copy
	TEST(VoltString, Copy)
	{
		String s("hello world");
		char buf[6] = {};
		s.copy(buf, 5, 6);
		ASSERT_STREQ(buf, "world");
	}

	// validate
	TEST(VoltString, Validate)
	{
		String s("hello");
		ASSERT_TRUE(s.validate());

		String empty;
		ASSERT_TRUE(empty.validate());
	}

	// String view conversion
	TEST(VoltString, StringViewConversion)
	{
		String s("hello");
		BasicStringView<char> sv = s;
		ASSERT_EQ(sv.size(), 5);
		ASSERT_EQ(sv.data(), s.data());
	}

	// Transition from SSO to heap
	TEST(VoltString, SSOToHeapTransition)
	{
		String s;
		// Append chars one at a time until we exceed SSO capacity
		for (int i = 0; i < 50; ++i)
		{
			s.push_back('a');
			ASSERT_TRUE(s.validate());
		}
		ASSERT_EQ(s.size(), 50);
	}

	// Multiple operations in sequence
	TEST(VoltString, MultipleOperations)
	{
		String s("hello");
		s += " world";
		s.replace(0, 5, "goodbye");
		s.erase(7);
		s.append("!");
		ASSERT_STREQ(s.c_str(), "goodbye!");
	}

	// WString basic functionality
	TEST(VoltString, WStringBasic)
	{
		WString s(L"hello");
		ASSERT_EQ(s.size(), 5);
		ASSERT_EQ(s[0], L'h');
		ASSERT_EQ(s[4], L'o');
		ASSERT_TRUE(s.validate());
	}
}

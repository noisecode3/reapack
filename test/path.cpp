#include "helper.hpp"

#include <path.hpp>

using Catch::Matchers::StartsWith;

using namespace std;

static const char *M = "[path]";

TEST_CASE("compare paths", M) {
  const Path a = Path("hello");
  const Path b = Path("world");

  REQUIRE_FALSE(a == b);
  REQUIRE(a != b);

  REQUIRE(a == a);
  REQUIRE_FALSE(a != a);
}

TEST_CASE("append path components", M) {
  Path path;
  REQUIRE(path.empty());
  REQUIRE(path.size() == 0);
  REQUIRE(path.join() == string());
  REQUIRE(path.dirname().empty());

  path.append("hello");
  REQUIRE_FALSE(path.empty());
  REQUIRE(path.size() == 1);
  REQUIRE(path.join() == "hello");
  REQUIRE(path.dirname().empty());

  path.append("world");
  REQUIRE(path.size() == 2);
#ifndef _WIN32
  REQUIRE(path.join() == "hello/world");
#else
  REQUIRE(path.join() == "hello\\world");
#endif
  REQUIRE(path.dirname().join() == "hello");

  path.append("test");
  REQUIRE(path.size() == 3);
#ifndef _WIN32
  REQUIRE(path.join() == "hello/world/test");
#else
  REQUIRE(path.join() == "hello\\world\\test");
#endif
  REQUIRE(path.dirname() == Path("hello/world"));
}

TEST_CASE("concatenate paths", M) {
  const Path a("hello");
  const Path b("world");

  Path c;
  c.append("hello");
  c += "world";

  REQUIRE(a + b == c);
  REQUIRE(a + "world" == c);
}

TEST_CASE("empty components", M) {
  Path a;
  a.append(string());

  REQUIRE(a.size() == 0);
}

TEST_CASE("strip trailing/leading slashes", M) {
  Path a;
  a.append("a/b/");
  a.append("/c/d/");
  a.append(Path("/e/f/"));

  REQUIRE(a.size() == 6);
#ifndef _WIN32
  REQUIRE(a.join() == "a/b/c/d/e/f");
#else
  REQUIRE(a.join() == "a\\b\\c\\d\\e\\f");
#endif
}

TEST_CASE("clear path", M) {
  Path a;
  a.append("test");

  CHECK(a.size() == 1);
  a.clear();
  REQUIRE(a.size() == 0);
}

TEST_CASE("modify path", M) {
  Path a;
  a.append("hello");

  a[0] = "world";
  REQUIRE(a.join() == "world");
}

TEST_CASE("custom separator", M) {
  Path a;
  a.append("hello");
  a.append("world");

  REQUIRE(a.join('-') == "hello-world");
}

TEST_CASE("split input", M) {
  SECTION("slash") {
    const Path a("hello/world");

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == "hello");
    REQUIRE(a[1] == "world");
  }

  SECTION("backslash") {
    const Path a("hello\\world");

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == "hello");
    REQUIRE(a[1] == "world");
  }

  SECTION("append") {
    Path a;
    a.append("hello/world");
    REQUIRE(a.size() == 2);

    a += "chunky/bacon";
    REQUIRE(a.size() == 4);
  }

  SECTION("skip empty parts") {
    const Path a("hello//world/");

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == "hello");
    REQUIRE(a[1] == "world");
  }
}

TEST_CASE("absolute path", M) {
  CHECK_FALSE(Path("a/b").absolute());

#ifdef _WIN32
  const Path a("C:\\Windows\\System32");
#else
  const Path a("/usr/bin/zsh");
#endif

  REQUIRE(a.absolute());
  CHECK(a.size() == 3);

#ifdef _WIN32
  CHECK(a[0] == "C:");
  CHECK(a.join() == "C:\\Windows\\System32");
#else
  CHECK(a[0] == "usr");
  CHECK(a.join() == "/usr/bin/zsh");
#endif
}

TEST_CASE("append absolute path to empty path", M) {
#ifdef _WIN32
  const Path abs("C:\\Windows\\");
#else
  const Path abs("/usr/bin");
#endif

  Path path;
  path += abs;

  CHECK(path == abs);
  REQUIRE(path.absolute());
}

TEST_CASE("extended absolute paths", M) {
#ifdef _WIN32
  Path abs("C:\\");
  abs.append(string(260, 'a'));

  CHECK(abs.absolute());
  REQUIRE_THAT(abs.join(), StartsWith("\\\\?\\"));

  const Path path(string(500, 'a'));
  CHECK_FALSE(path.absolute());
#else
  Path path("/hello");
  path.append(string(260, 'a'));
#endif

  REQUIRE_THAT(path.join(), !StartsWith("\\\\?\\"));
}

#ifndef _WIN32
TEST_CASE("compare absolute to relative path (unix)", M) {
  REQUIRE(Path("/a/b") != Path("a/b"));
}
#endif

TEST_CASE("remove last component of path", M) {
  Path a;
  a.append("a");
  a.append("b");

  CHECK(a.size() == 2);

  a.removeLast();

  REQUIRE(a.size() == 1);
  REQUIRE(a[0] == "a");

  a.removeLast();
  REQUIRE(a.empty());

  a.removeLast(); // no crash
}

TEST_CASE("prepend root path", M) {
  const Path path("world");

  REQUIRE(path.prependRoot() == Path("world"));

  {
    UseRootPath root(Path("hello"));
    (void)root;

    REQUIRE(path.prependRoot() == Path("hello/world"));
  }

  REQUIRE(path.prependRoot() == Path("world"));
}

TEST_CASE("remove root path", M) {
  const Path path("hello/world");

  REQUIRE(path.removeRoot() == Path("hello/world"));

  {
    UseRootPath root(Path("hello"));
    (void)root;

    REQUIRE(path.removeRoot() == Path("world"));
  }

  REQUIRE(path.removeRoot() == Path("hello/world"));
}

TEST_CASE("first and last path component", M) {
  REQUIRE(Path().first().empty());
  REQUIRE(Path().last().empty());

  const Path path("hello/world/chunky/bacon");
  REQUIRE(path.first() == "hello");
  REQUIRE(path.last() == "bacon");
}

TEST_CASE("directory traversal", M) {
  SECTION("append (enabled)") {
    REQUIRE(Path("a/./b") == Path("a/b"));
    REQUIRE(Path("a/../b") == Path("b"));
    REQUIRE(Path("../a") == Path("a"));
  }

  SECTION("append (disabled)") {
    Path a;
    a.append("a/../b", false);
    REQUIRE(a == Path("a/b"));
  }

  SECTION("concatenate") {
    // concatenating a std::string to a path, directory traversal is applied
    const Path a = Path("a/b/c") + "../../../d/e/f";
    REQUIRE(a == Path("d/e/f"));

    // here, the directory traversal components are lost before concatenating
    const Path b = Path("a/b/c") + Path("../../../d/e/f");
    REQUIRE(b == Path("a/b/c/d/e/f"));
  }
}

TEST_CASE("append full paths", M) {
  Path a;
  a += Path("a/b");
  a.append(Path("c/d"));
  a.append(Path("e/f"));

  REQUIRE(a == Path("a/b/c/d/e/f"));
}

TEST_CASE("temporary path", M) {
  TempPath a(Path("hello/world"));

  REQUIRE(a.target() == Path("hello/world"));
  REQUIRE(a.temp() == Path("hello/world.part"));
}

TEST_CASE("path starts with", M) {
  const Path ref("a/b");

  REQUIRE(ref.startsWith(ref));
  REQUIRE(Path("a/b/c").startsWith(ref));
#ifndef _WIN32
  REQUIRE_FALSE(Path("/a/b/c").startsWith(ref));
#endif
  REQUIRE_FALSE(Path("0/a/b/c").startsWith(ref));
  REQUIRE_FALSE(Path("a").startsWith(ref));
}

TEST_CASE("remove path segments", M) {
  Path path("/a/b/c/d/e");

  SECTION("remove from start") {
    path.remove(0, 1);
    REQUIRE(path == Path("b/c/d/e"));
    REQUIRE_FALSE(path.absolute());
  }

  SECTION("remove from middle") {
    path.remove(1, 2);
    REQUIRE(path == Path("/a/d/e"));
#ifndef _WIN32
    REQUIRE(path.absolute());
#endif
  }

  SECTION("remove past the end") {
    path.remove(4, 2);
    path.remove(18, 1);
    REQUIRE(path == Path("/a/b/c/d"));
  }
}

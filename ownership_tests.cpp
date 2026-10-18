#include "ownership_file.hpp"

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
namespace scheduler = biglybtc::plugins::speed_scheduler;
void check(bool value, const char* message) { if (!value) { std::cerr << message << '\n'; std::exit(1); } }
std::array<std::uint8_t, 20> identity(std::uint8_t value) {
  std::array<std::uint8_t, 20> result{};
  result.fill(value);
  return result;
}
}
int main(int argc, char** argv) {
  const auto hash = identity(0xab);
  const auto formatted = scheduler::originalOwnershipHash(hash).value();
  check(formatted == "ABABABAB ABABABAB ABABABAB ABABABAB ABABABAB", "Original grouped uppercase hash format changed");
  if (argc == 3 && std::string_view(argv[1]) == "--original-file") {
    const std::string path = argv[2];
    check(path.starts_with("/tmp/biglybtc-"), "Only a generated fixture is accepted");
    std::ifstream stream(path, std::ios::binary);
    check(stream.good(), "Cannot read original ownership fixture");
    stream.seekg(0, std::ios::end);
    check(stream.tellg() >= 0 && stream.tellg() <= static_cast<std::streamoff>(scheduler::OwnershipFile::MaximumBytes), "Fixture exceeds byte bound");
    stream.seekg(0);
    auto document = scheduler::OwnershipFile::read({std::istreambuf_iterator<char>(stream), {}});
    check(document.has_value(), "Cannot parse original ownership fixture");
    std::cout << "FORMAT " << formatted << '\n';
    std::cout << "BEFORE " << document->contains(hash).value() << '\n';
    auto appended = document->afterPause(hash);
    if (!appended) { std::cout << "APPEND_REJECTED\n"; return 0; }
    auto after = scheduler::OwnershipFile::read(*appended).value();
    std::cout << "APPENDED " << after.contains(hash).value() << '\n';
    auto removed = scheduler::OwnershipFile::read(after.afterResume(hash).value()).value();
    std::cout << "REMOVED " << removed.contains(hash).value() << '\n';
    return 0;
  }
  check(argc == 1, "Unsupported ownership test arguments");
  auto empty = scheduler::OwnershipFile::read("").value();
  check(!empty.contains(hash).value(), "Empty file owns a torrent");
  const auto appended = empty.afterPause(hash).value();
  check(appended == formatted + '\n', "Original Linux append encoding changed");
  check(empty.afterPause(hash, true).value() == formatted + "\r\n", "Original Windows newline encoding changed");
  auto owned = scheduler::OwnershipFile::read(appended).value();
  check(owned.contains(hash).value() && owned.afterPause(hash).value() == appended, "Existing ownership was duplicated");
  check(owned.afterResume(hash).value().empty(), "Resume did not remove ownership");
  const auto raw = std::string("future \xff bytes\r\n\n \t") + formatted + " \t\r\n" + formatted + "\r";
  auto duplicates = scheduler::OwnershipFile::read(raw).value();
  check(duplicates.bytes() == raw && duplicates.contains(hash).value(), "Read did not retain unknown raw bytes/trim ownership");
  const auto firstRemoved = duplicates.afterResume(hash).value();
  check(firstRemoved == std::string("future \xff bytes\r\n\n") + formatted + '\r', "Resume rewrote unrelated original bytes");
  auto remaining = scheduler::OwnershipFile::read(firstRemoved).value();
  check(remaining.contains(hash).value(), "Original first-only duplicate removal changed");
  check(remaining.afterResume(hash).value() == "future \xff bytes\r\n\n", "Second duplicate did not remove cleanly");
  const auto other = identity(0xcd);
  check(duplicates.afterResume(other).value() == raw, "Absent ownership removal rewrote the file");
  const auto unterminated = scheduler::OwnershipFile::read("future unterminated").value();
  check(!unterminated.afterPause(hash), "Acknowledged an append Java cannot find");
  std::array<std::uint8_t, 32> v2{};
  check(scheduler::originalOwnershipHash(v2)->size() == 71, "32-byte formatter geometry changed");
  check(!scheduler::originalOwnershipHash(std::span<const std::uint8_t>{}), "Empty ownership hash accepted");
  auto oversized = std::string(scheduler::OwnershipFile::MaximumBytes + 1, 'x');
  check(!scheduler::OwnershipFile::read(std::move(oversized)), "Oversized ownership input accepted");
  auto many = scheduler::OwnershipFile::read(std::string(scheduler::OwnershipFile::MaximumLines, '\n')).value();
  check(!many.afterPause(hash), "Ownership append exceeded its line bound");
  check(!scheduler::OwnershipFile::read(std::string(scheduler::OwnershipFile::MaximumLines + 1, '\n')), "Excess original lines accepted");
  std::string library;
  library.reserve(50000 * 45);
  auto numbered = [](std::uint32_t number) {
    std::array<std::uint8_t, 20> value{};
    for (int i = 0; i < 4; ++i) { value[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(number >> (i * 8)); }
    return value;
  };
  for (std::uint32_t i = 0; i < 50000; ++i) { library += scheduler::originalOwnershipHash(numbered(i)).value() + '\n'; }
  const auto indexed = scheduler::OwnershipFile::read(std::move(library)).value();
  for (std::uint32_t i = 0; i < 50000; ++i) { check(indexed.contains(numbered(i)).value(), "Large original ownership lookup lost a record"); }
  check(!indexed.contains(numbered(50000)).value(), "Large ownership lookup invented a record");
  std::cout << "PASS original pause ownership bounds, duplicates, line endings and raw-byte preservation\n";
}

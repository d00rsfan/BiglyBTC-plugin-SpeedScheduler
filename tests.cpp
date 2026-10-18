#include "scheduler.hpp"

#include <QByteArray>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>

namespace {
namespace scheduler = biglybtc::plugins::speed_scheduler;
void check(bool value, const char* message) { if (!value) { std::cerr << message << '\n'; std::exit(1); } }
std::string content(const std::string& path) {
  check(path.starts_with("/tmp/biglybtc-"), "Only generated test XML paths are accepted");
  std::ifstream file(path, std::ios::binary);
  check(file.good(), "Cannot open generated XML fixture");
  file.seekg(0, std::ios::end);
  check(file.tellg() >= 0 && file.tellg() <= 4 * 1024 * 1024, "Generated XML/property fixture exceeds its bound");
  file.seekg(0);
  return {std::istreambuf_iterator<char>(file), {}};
}
void output(const scheduler::Configuration& configuration) {
  std::cout << "CONFIG " << configuration.defaultUpload << ' ' << configuration.defaultDownload << ' '
      << configuration.defaultsEnforce << ' ' << configuration.schedules.size() << '\n';
  for (std::size_t i = 0; i < configuration.schedules.size(); ++i) {
    const auto& s = configuration.schedules[i];
    std::cout << "SCHEDULE " << i << ' ' << s.start << ' ' << s.end << ' ' << s.upload << ' ' << s.download << ' '
        << s.downloadsPaused << ' ' << s.seedsPaused << ' ' << s.enabled << ' ' << s.excludeCategory << ' '
        << s.onlyCategory << ' ' << s.category.has_value() << ' '
        << (s.category ? QByteArray::fromStdString(*s.category).toHex().toStdString() : "-") << ' ';
    for (bool selected : s.days) { std::cout << selected; }
    std::cout << '\n';
    auto allDays = s;
    allDays.days.fill(true);
    std::cout << "SPAN " << i << ' ';
    for (int minute = 0; minute < 1440; ++minute) { std::cout << scheduler::inSpan(allDays, {0, minute}); }
    std::cout << '\n';
  }
}
void tick(const scheduler::Configuration& configuration, const std::string& path) {
  std::map<std::string, std::string> values;
  const auto bytes = content(path);
  check(bytes.size() <= 4 * 1024 * 1024, "Generated policy fixture exceeds its bound");
  std::size_t start = 0;
  while (start < bytes.size()) {
    const auto end = bytes.find('\n', start);
    auto line = bytes.substr(start, end == std::string::npos ? end : end - start);
    if (line.ends_with('\r')) { line.pop_back(); }
    const auto split = line.find('=');
    if (split != std::string::npos) { values.emplace(line.substr(0, split), line.substr(split + 1)); }
    if (end == std::string::npos) { break; }
    start = end + 1;
  }
  auto number = [&](const std::string& key) { return std::stoi(values.at(key)); };
  auto flag = [&](const std::string& key) { return values.at(key) == "true"; };
  std::vector<scheduler::Torrent> torrents;
  const auto count = number("torrent.count");
  check(count >= 0 && count <= 1000, "Generated torrent fixture exceeds its bound");
  for (int i = 0; i < count; ++i) {
    const auto key = "torrent." + std::to_string(i) + '.';
    const auto category = values.find(key + "category");
    torrents.push_back({.id = values.at(key + "id"), .complete = flag(key + "complete"),
        .state = static_cast<scheduler::TorrentState>(number(key + "state")), .forceStarted = flag(key + "forced"),
        .paused = flag(key + "paused"), .ownedPause = flag(key + "owned"),
        .memberships = {category == values.end() ? "Uncategorized" : category->second}});
  }
  auto result = scheduler::evaluate(configuration, {1, 600}, torrents,
      {.effectiveUpload = number("upload"), .globalDownload = number("download"),
          .nonDefaultUploadSet = flag("nonDefaultUpload"), .nonDefaultDownloadSet = flag("nonDefaultDownload")},
      flag("enabled"), flag("override"));
  check(result.has_value(), result ? "" : result.error().message.c_str());
  std::cout << "STATE " << result->nonDefaultUploadSet << ' ' << result->nonDefaultDownloadSet << '\n';
  std::cout << "SELECT";
  for (auto index : result->selectedSchedules) { std::cout << ' ' << index; }
  std::cout << '\n';
  std::map<std::string, bool> owned;
  for (const auto& torrent : torrents) { owned.emplace(torrent.id, torrent.ownedPause); }
  for (const auto& action : result->actions) {
    using scheduler::ActionKind;
    const auto kind = action.kind == ActionKind::Pause ? 'P' : action.kind == ActionKind::Resume ? 'R'
        : action.kind == ActionKind::Restart ? 'T' : 'S';
    std::cout << "ACTION " << kind << ' ' << action.id << '\n';
    if (action.kind == ActionKind::Pause) {
      std::cout << "ACTION S " << action.id << '\n';
      owned.at(action.id) = true;
    } else if (action.kind == ActionKind::Resume || action.kind == ActionKind::Restart) { owned.at(action.id) = false; }
  }
  if (result->effectiveUpload) { std::cout << "RATE U " << *result->effectiveUpload << '\n'; }
  if (result->globalDownload) { std::cout << "RATE D " << *result->globalDownload << '\n'; }
  for (const auto& torrent : torrents) { std::cout << "OWNED " << torrent.id << ' ' << owned.at(torrent.id) << '\n'; }
}
}

int main(int argc, char** argv) {
  using namespace scheduler;
  if (argc == 3 && std::string_view(argv[1]) == "--xml") {
    auto parsed = readOriginalXml(content(argv[2]));
    check(parsed.has_value(), parsed ? "" : parsed.error().message.c_str());
    output(parsed->configuration);
    return 0;
  }
  if (argc == 4 && std::string_view(argv[1]) == "--tick") {
    auto parsed = readOriginalXml(content(argv[2]));
    check(parsed.has_value(), parsed ? "" : parsed.error().message.c_str());
    tick(parsed->configuration, argv[3]);
    return 0;
  }
  Schedule rule;
  rule.days[1] = true;
  rule.start = 22 * 60;
  rule.end = 2 * 60;
  check(inSpan(rule, {1, 120}) && inSpan(rule, {1, 1320}) && inSpan(rule, {1, 600}), "Original reversed closed interval differs");
  check(!inSpan(rule, {1, 60}) && !inSpan(rule, {1, 1380}) && !inSpan(rule, {2, 600}), "Original reversed interval must not become an overnight wrap");
  rule.start = rule.end = 600;
  check(inSpan(rule, {1, 600}) && !inSpan(rule, {1, 599}) && !inSpan(rule, {1, 601}), "Equal endpoints select one minute");
  rule.start = 0; rule.end = 1439; rule.upload = 10; rule.download = 50; rule.downloadsPaused = true;
  Configuration config{.defaultUpload = 7, .defaultDownload = 9, .schedules = {rule}};
  Schedule unlimited = rule; unlimited.upload = 0; unlimited.download = 0; unlimited.downloadsPaused = false;
  config.schedules.push_back(unlimited);
  std::vector<Torrent> torrents{{.id = "active", .state = TorrentState::Downloading},
      {.id = "manual", .state = TorrentState::Stopped, .paused = true},
      {.id = "forced", .state = TorrentState::Downloading, .forceStarted = true},
      {.id = "owned", .state = TorrentState::Stopped, .paused = true, .ownedPause = true}};
  const std::vector<Action> expectedPaused{{"active", ActionKind::Pause}, {"owned", ActionKind::SetStopReason}};
  const std::vector<Action> expectedResume{{"owned", ActionKind::Resume}};
  auto active = evaluate(config, {1, 600}, torrents, {});
  check(active && active->effectiveUpload == 10 && active->globalDownload == 50,
      "Original overlaps choose largest numeric rate including zero");
  check(active->actions == expectedPaused,
      "Manual pauses and forced torrents must retain ownership");
  auto override = evaluate(config, {1, 600}, torrents, {}, true, true);
  check(override && override->actions.size() == 3 && override->actions[1] == Action{"forced", ActionKind::Pause}, "Forced override differs");
  auto ended = evaluate(config, {2, 600}, torrents, {.effectiveUpload = 10, .globalDownload = 50,
      .nonDefaultUploadSet = true, .nonDefaultDownloadSet = true});
  check(ended && ended->actions == expectedResume
      && ended->effectiveUpload == 7 && ended->globalDownload == 9 && !ended->nonDefaultUploadSet,
      "Ending schedule must restore defaults and resume only owned pauses");
  config.defaultsEnforce = false;
  auto startup = evaluate(config, {2, 600}, {}, {.effectiveUpload = 20, .globalDownload = 30});
  check(startup && startup->effectiveUpload == 7 && startup->globalDownload == 9, "Original fresh thread must restore defaults once");
  auto untouched = evaluate(config, {2, 600}, {}, {.effectiveUpload = 20, .globalDownload = 30,
      .nonDefaultUploadSet = false, .nonDefaultDownloadSet = false});
  check(untouched && !untouched->effectiveUpload && !untouched->globalDownload, "Unenforced defaults must respect user changes");
  rule.excludeCategory = true; rule.category = "keep";
  config.schedules = {rule};
  torrents[0].memberships = {"keep"}; torrents[3].memberships = {"keep"};
  auto excluded = evaluate(config, {1, 600}, torrents, {});
  check(excluded && excluded->actions == expectedResume, "Original exclusion membership differs");
  rule.excludeCategory = false; rule.onlyCategory = true;
  config.schedules = {rule};
  auto only = evaluate(config, {1, 600}, torrents, {});
  check(only && only->actions == expectedPaused, "Original category-only ownership differs");
  auto disabled = evaluate(config, {1, 600}, torrents, {}, false);
  check(disabled && disabled->actions == expectedResume
      && !disabled->effectiveUpload, "Disabled polling must only release plugin-owned pauses");
  torrents[1].id = torrents[0].id;
  check(!evaluate(config, {1, 600}, torrents, {}), "Ambiguous torrent identity must not receive commands");
  const std::string xml = "<?xml version=\"1.0\"?><SpeedScheduler future=\"yes\"><!--retain--><defaultRates upload=\"7\" download=\"9\"/>"
      "<schedules><schedule><startTime hour=\"22\" minute=\"0\"/><endTime hour=\"2\" minute=\"0\"/>"
      "<rates upload=\"10\" download=\"50\" pauseTransfers=\"TrUe\"/><day index=\"1\" selected=\"true\"/>"
      "<category selected=\"ש&amp;ب\"/><future>opaque</future></schedule></schedules></SpeedScheduler>";
  auto imported = readOriginalXml(xml);
  check(imported && imported->bytes == xml && imported->configuration.defaultsEnforce
      && imported->configuration.schedules.size() == 1 && imported->configuration.schedules[0].seedsPaused
      && imported->configuration.schedules[0].category == "ש&ب", "Original XML/legacy pause/unknown bytes differ");
  for (const auto& bad : {"<!DOCTYPE SpeedScheduler [<!ENTITY a 'x'>]><SpeedScheduler/>",
      "<SpeedScheduler><schedule><day index='7' selected='true'/></schedule></SpeedScheduler>",
      "<SpeedScheduler><schedule><startTime hour='24' minute='0'/></schedule></SpeedScheduler>",
      "<SpeedScheduler><schedule><schedule/></schedule></SpeedScheduler>", "<SpeedScheduler>"}) {
    check(!readOriginalXml(bad), "Malformed/unbounded original schedule XML accepted");
  }
  std::cout << "PASS original scheduler intervals, overlaps, categories, ownership, force, defaults and byte-preserving XML\n";
}

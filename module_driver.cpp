#include "biglybtc/plugin_host.hpp"
#include "biglybtc/scheduler.h"
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <vector>
void require(bool value) { if (!value) { throw std::runtime_error("Scheduler module contract failed"); } }
std::string read(const std::string& path) {
  require(path.starts_with("/tmp/biglybtc-"));
  std::ifstream stream(path, std::ios::binary);
  require(stream.good());
  return {std::istreambuf_iterator<char>(stream), {}};
}
int main(int argc, char** argv) {
  require(argc == 4);
  const auto xml = read(argv[2]);
  const auto bytes = read(argv[3]);
  std::map<std::string, std::string> values;
  std::size_t offset = 0;
  while (offset < bytes.size()) {
    const auto end = bytes.find('\n', offset);
    auto line = bytes.substr(offset, end == std::string::npos ? end : end - offset);
    if (line.ends_with('\r')) { line.pop_back(); }
    const auto split = line.find('=');
    if (split != std::string::npos) { values.emplace(line.substr(0, split), line.substr(split + 1)); }
    if (end == std::string::npos) { break; }
    offset = end + 1;
  }
  auto number = [&](const std::string& key) { return std::stoi(values.at(key)); };
  auto flag = [&](const std::string& key) { return values.at(key) == "true"; };
  const auto count = number("torrent.count");
  require(count >= 0 && count <= 100000);
  std::vector<bt_scheduler_torrent_v1> torrents;
  std::vector<const char*> categories(static_cast<std::size_t>(count));
  std::vector<std::string> ids;
  std::vector<bool> owned;
  for (int i = 0; i < count; ++i) {
    const auto key = "torrent." + std::to_string(i) + '.';
    const auto category = values.find(key + "category");
    categories[i] = category == values.end() ? "Uncategorized" : category->second.c_str();
    torrents.push_back({static_cast<std::uint64_t>(i), static_cast<std::uint32_t>(number(key + "state")),
        (flag(key + "complete") ? BT_TORRENT_COMPLETE : 0) | (flag(key + "forced") ? BT_TORRENT_FORCED : 0)
        | (flag(key + "paused") ? BT_TORRENT_PAUSED : 0) | (flag(key + "owned") ? BT_TORRENT_OWNED_PAUSE : 0),
        &categories[i], 1, 0});
    ids.push_back(values.at(key + "id")); owned.push_back(flag(key + "owned"));
  }
  bt_scheduler_request_v1 request{reinterpret_cast<const std::uint8_t*>(xml.data()),
      static_cast<std::uint32_t>(xml.size()), 1, 600,
      (flag("enabled") ? BT_SCHEDULER_ENABLED : 0) | (flag("override") ? BT_SCHEDULER_OVERRIDE_FORCED : 0)
      | (flag("nonDefaultUpload") ? BT_SCHEDULER_NONDEFAULT_UPLOAD : 0)
      | (flag("nonDefaultDownload") ? BT_SCHEDULER_NONDEFAULT_DOWNLOAD : 0),
      number("upload"), number("download"), torrents.data(), static_cast<std::uint32_t>(torrents.size()), 0};
  const int directory = ::open("/tmp", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  const bt_plugin_options_v1 options{sizeof(options), 1, directory, 0, "1.9.2", nullptr, nullptr};
  auto module = biglybtc::plugins::Module::load(argv[1], "SpeedScheduler", BT_CAP_SCHEDULER, options);
  ::close(directory);
  require(module.has_value());
  bt_scheduler_result_v1 result{};
  auto status = module->call(BT_SCHEDULER_EVALUATE, &request, sizeof(request), &result, sizeof(result));
  require(status == BT_OK || status == BT_BUFFER_SMALL);
  std::vector<bt_scheduler_action_v1> actions(result.action_count);
  result.actions = actions.data(); result.action_capacity = static_cast<std::uint32_t>(actions.size());
  require(module->call(BT_SCHEDULER_EVALUATE, &request, sizeof(request), &result, sizeof(result)) == BT_OK);
  std::cout << "STATE " << bool(result.flags & BT_SCHEDULER_NONDEFAULT_UPLOAD) << ' '
      << bool(result.flags & BT_SCHEDULER_NONDEFAULT_DOWNLOAD) << '\n';
  for (const auto& action : actions) {
    require(action.token < ids.size() && action.kind >= 1 && action.kind <= 4 && !action.reserved);
    const char kinds[] = " PRTS";
    std::cout << "ACTION " << kinds[action.kind] << ' ' << ids[action.token] << '\n';
    if (action.kind == BT_SCHEDULER_PAUSE) {
      std::cout << "ACTION S " << ids[action.token] << '\n'; owned[action.token] = true;
    } else if (action.kind == BT_SCHEDULER_RESUME || action.kind == BT_SCHEDULER_RESTART) { owned[action.token] = false; }
  }
  if (result.flags & BT_SCHEDULER_UPLOAD_CHANGED) { std::cout << "RATE U " << result.effective_upload << '\n'; }
  if (result.flags & BT_SCHEDULER_DOWNLOAD_CHANGED) { std::cout << "RATE D " << result.global_download << '\n'; }
  for (std::size_t i = 0; i < ids.size(); ++i) { std::cout << "OWNED " << ids[i] << ' ' << owned[i] << '\n'; }
  request.flags |= 0x80000000;
  require(module->call(BT_SCHEDULER_EVALUATE, &request, sizeof(request), &result, sizeof(result)) == BT_INVALID);
}

// SPDX-License-Identifier: Proprietary
// CodesysBridge core — pure C++, no FreeCAD or Qt dependency.

#include "CodesysBridge.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace MilCAM {

struct CodesysBridge::Impl {
    std::string  endpointUrl = "opc.tcp://192.168.1.123:4840";
    std::string  dropFolder  = "/var/cnc/jobs";
    bool         connected   = false;
    MachineState state       = MachineState::Unknown;
    int          currentLine = 0;
};

CodesysBridge::CodesysBridge() : impl_(new Impl) {}
CodesysBridge::~CodesysBridge() { delete impl_; }

void CodesysBridge::setEndpointUrl(const std::string& url) { impl_->endpointUrl = url; }
void CodesysBridge::setDropFolder (const std::string& p)   { impl_->dropFolder  = p; }
const std::string& CodesysBridge::endpointUrl() const { return impl_->endpointUrl; }
const std::string& CodesysBridge::dropFolder()  const { return impl_->dropFolder; }

std::string CodesysBridge::dropGCode(const std::string& jobId, const std::string& gcode) {
    std::error_code ec;
    fs::create_directories(impl_->dropFolder, ec);
    if (ec) return {};

    const fs::path finalPath = fs::path(impl_->dropFolder) / (jobId + ".gcode");
    const fs::path tmpPath   = finalPath;
    fs::path tmp = tmpPath; tmp += ".tmp";

    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) return {};
        const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char buf[32]; std::strftime(buf, sizeof(buf), "%FT%TZ", std::gmtime(&t));
        out << "; MilCAM job " << jobId << '\n'
            << "; generated " << buf << '\n'
            << gcode;
    }
    // Atomic rename — POSIX guarantees readers see either old or new file.
    fs::remove(finalPath, ec);
    fs::rename(tmp, finalPath, ec);
    if (ec) { fs::remove(tmp); return {}; }
    return finalPath.string();
}

int CodesysBridge::pruneOldJobs(int days) {
    if (days <= 0) return 0;
    const auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * days);
    int removed = 0;
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(impl_->dropFolder, ec)) {
        if (ec) break;
        if (entry.path().extension() != ".gcode") continue;
        auto ftime = fs::last_write_time(entry, ec);
        if (ec) continue;
        const auto sctp = std::chrono::file_clock::to_sys(ftime);
        if (sctp < cutoff) {
            fs::remove(entry.path(), ec);
            if (!ec) ++removed;
        }
    }
    return removed;
}

// ── OPC UA stubs — filled in Faz 3 with open62541 ────────────────────
bool CodesysBridge::connect()    { impl_->connected = false; return false; }
void CodesysBridge::disconnect() { impl_->connected = false; }
bool CodesysBridge::isConnected() const { return impl_->connected; }
bool CodesysBridge::notifyJobReady(const std::string&) { return false; }
MachineState CodesysBridge::machineState() const { return impl_->state; }
int CodesysBridge::currentLine() const { return impl_->currentLine; }

bool CodesysBridge::raiseWindow(const std::string& titleSubstring) {
    // Cheap shell exec — wmctrl returns 0 on success.
    const std::string cmd = "wmctrl -a \"" + titleSubstring + "\" 2>/dev/null";
    return std::system(cmd.c_str()) == 0;
}

} // namespace MilCAM

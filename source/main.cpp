#include <fmt/base.h>
#include <fmt/chrono.h>
#include <3ds.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
    #include "ptmplays.h"
}

namespace {

    std::vector<PtmPlayEvent> GetPlayEvents() {
        std::vector<PtmPlayEvent> ret;

        s32 startIndex = 0;
        s32 endIndex = 0;

        Result res = ptmPlaysInit();
        if (R_FAILED(res)) {
            fmt::print("ptmPlaysInit failure: {:08X}\n", static_cast<u32>(res));
            return ret;
        }
        struct PtmPlaysServiceGuard {
            // Defer
            ~PtmPlaysServiceGuard() noexcept { ptmPlaysExit(); }
        } ptmPlaysServiceGuard{};

        // Get start index
        res = PTMPLAYS_GetPlayHistoryStart(&startIndex);
        if (R_FAILED(res)) {
            fmt::print("PTMPLAYS_GetPlayHistoryStart failure: {:08X}\n", static_cast<u32>(res));
            return ret;
        }

        // Get total size
        s32 totalSize = 0;
        res = PTMPLAYS_GetPlayHistorySize(&totalSize);
        if (R_FAILED(res)) {
            fmt::print("PTMPLAYS_GetPlayHistorySize failure: {:08X}\n", static_cast<u32>(res));
            return ret;
        }

        if (totalSize < 0) {
            svcBreak(USERBREAK_PANIC);
        }

        ret.resize(static_cast<size_t>(totalSize));

        res = PTMPLAYS_GetPlayHistory(&endIndex, ret.data(), startIndex, totalSize);
        if (R_FAILED(res)) {
            fmt::print("PTMPLAYS_GetPlayHistory failure: {:08X}\n", static_cast<u32>(res));
        }

        //fmt::print("Start index: {:#x}\n", startIndex);

        return ret;
    }

    std::string FormatTimestamp(u32 minutesSince2000) {
        using namespace std::chrono;
        constexpr auto kBase = sys_days{year{2000}/January/1};
        const auto timePoint = kBase + minutes{static_cast<int64_t>(minutesSince2000)};
        return fmt::format("{:%Y-%m-%d %H:%M}", timePoint);
    }

    std::string FormatTitleId(u64 titleId) {
        if (titleId == PTM_INVALID_TITLE_ID) {
            return "(invalid TitleId)";
        }

        return fmt::format("({:016X})", titleId);
    }

    void AppendStandardEvent(fmt::memory_buffer &buffer, const PtmPlayEvent &event) {
        const std::string timestamp = FormatTimestamp(event.minutesSince2000);
        const u64 tid = ptmGetPlayEventTitleId(event);
        const std::string titleId = FormatTitleId(tid);

        switch (event.type) {
            case PTMPLAYEVENT_APPLICATION_LAUNCH: {
                if (tid == PTM_INVALID_TITLE_ID) {
                    fmt::format_to(std::back_inserter(buffer), "{}: DSi application start\n", timestamp);
                } else {
                    fmt::format_to(std::back_inserter(buffer), "{}: Application launch {}\n", timestamp, titleId);
                }
                break;
            }
            case PTMPLAYEVENT_APPLICATION_EXIT: {
                if (tid == PTM_INVALID_TITLE_ID) {
                    fmt::format_to(std::back_inserter(buffer), "{}: DSi application exit\n", timestamp);
                } else {
                    fmt::format_to(std::back_inserter(buffer), "{}: Application exit {}\n", timestamp, titleId);
                }
                break;
            }
            case PTMPLAYEVENT_APPLET_LAUNCH:
                fmt::format_to(std::back_inserter(buffer), "{}: Applet launch {}\n", timestamp, titleId);
                break;
            case PTMPLAYEVENT_APPLET_EXIT:
                fmt::format_to(std::back_inserter(buffer), "{}: Applet exit {}\n", timestamp, titleId);
                break;
            case PTMPLAYEVENT_JUMP_TO_APPLICATION:
                fmt::format_to(std::back_inserter(buffer), "{}: Jump to application {}\n", timestamp, titleId);
                break;
            case PTMPLAYEVENT_LEAVE_APPLICATION:
                fmt::format_to(std::back_inserter(buffer), "{}: Leave application {}\n", timestamp, titleId);
                break;
            case PTMPLAYEVENT_JUMP_TO_APPLET:
                fmt::format_to(std::back_inserter(buffer), "{}: Jump to applet {}\n", timestamp, titleId);
                break;
            case PTMPLAYEVENT_LEAVE_APPLET:
                fmt::format_to(std::back_inserter(buffer), "{}: Leave applet {}\n", timestamp, titleId);
                break;
            case PTMPLAYEVENT_SHELL_CLOSE:
                fmt::format_to(std::back_inserter(buffer), "{}: Shell close\n", timestamp);
                break;
            case PTMPLAYEVENT_SHELL_OPEN:
                fmt::format_to(std::back_inserter(buffer), "{}: Shell open\n", timestamp);
                break;
            case PTMPLAYEVENT_SYSTEM_SHUTDOWN:
                fmt::format_to(std::back_inserter(buffer), "{}: System shutdown\n", timestamp);
                break;
            case PTMPLAYEVENT_USER_TIME_CHANGE_OLD_TIME:
            case PTMPLAYEVENT_USER_TIME_CHANGE_NEW_TIME:
                // handled separately
                break;
        }
    }

    void ExportEvents(const std::vector<PtmPlayEvent> &events) {
        fmt::memory_buffer buffer{};

        for (size_t i = 0; i < events.size(); ++i) {
            const auto &event = events[i];
            if (event.type == PTMPLAYEVENT_USER_TIME_CHANGE_OLD_TIME) {
                if (i + 1 < events.size() && events[i + 1].type == PTMPLAYEVENT_USER_TIME_CHANGE_NEW_TIME) {
                    const auto &newEvent = events[i + 1];
                    fmt::format_to(std::back_inserter(buffer), "{}: User time change from {} to {}\n",
                        FormatTimestamp(newEvent.minutesSince2000),
                        FormatTimestamp(event.minutesSince2000),
                        FormatTimestamp(newEvent.minutesSince2000));
                    ++i; // consume the NEW_TIME event we just handled
                } else {
                    fmt::format_to(std::back_inserter(buffer), "{}: User time change (old={}, new time missing)\n",
                        FormatTimestamp(event.minutesSince2000),
                        FormatTimestamp(event.minutesSince2000));
                }
                continue;
            }

            if (event.type == PTMPLAYEVENT_USER_TIME_CHANGE_NEW_TIME) {
                fmt::format_to(std::back_inserter(buffer), "{}: User time change (new={}, old time missing)\n",
                    FormatTimestamp(event.minutesSince2000),
                    FormatTimestamp(event.minutesSince2000));
                continue;
            }

            AppendStandardEvent(buffer, event);
        }

        auto fcloser = [](FILE *f) noexcept { std::fclose(f); };

        std::unique_ptr<FILE, decltype(fcloser)> output{std::fopen("sdmc:/play_events.log", "w+")};
        if (!output) {
            fmt::print("Failed to open sdmc:/play_events.log: {}\n", std::strerror(errno));
            return;
        }

        const auto written = std::fwrite(buffer.data(), 1, buffer.size(), output.get());

        if (written != buffer.size()) {
            fmt::print("Failed to write all data to sdmc:/play_events.log ({} of {} bytes)\n",
                written,
                buffer.size());
            return;
        }

        fmt::print("Exported {} events to sdmc:/play_events.log\n", events.size());
    }
}

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    std::vector<PtmPlayEvent> events = GetPlayEvents();
    ExportEvents(events);

    // Main loop
    while (aptMainLoop()) {
        gspWaitForVBlank();
        gfxSwapBuffers();
        hidScanInput();

        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            break;
        }
    }

    gfxExit();
    return 0;
}

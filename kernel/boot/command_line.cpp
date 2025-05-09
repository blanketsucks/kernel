#include <kernel/boot/command_line.h>
#include <std/format.h>

namespace kernel {

CommandLine s_instance;

void CommandLine::init() {
    if (g_boot_info->cmdline == nullptr) {
        return;
    }

    s_instance.parse(g_boot_info->cmdline);
}

CommandLine* CommandLine::instance() {
    return &s_instance;
}

void CommandLine::parse(StringView cmdline) {
    size_t start = 0;
    size_t end = 0;

    while (end < cmdline.size()) {
        while (end < cmdline.size() && cmdline[end] != ' ') {
            end++;
        }

        StringView arg = cmdline.substr(start, end);
        size_t equal = arg.find('=');

        if (equal != StringView::npos) {
            StringView key = arg.substr(0, equal);
            StringView value = arg.substr(equal + 1);

            m_args.set(key, value);
        } else {
            m_args.set(arg, "");
        }

        start = ++end;
    }
}

Optional<StringView> CommandLine::get(StringView key) const {
    return m_args.get(key);
}

bool CommandLine::has(StringView key) const {
    return m_args.contains(key);
}

StringView CommandLine::root() const {
    return this->get("root").value_or("/dev/hda1");
}

}
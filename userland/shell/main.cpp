#include <shell/terminal.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

#include <std/format.h>
#include <std/types.h>
#include <std/vector.h>
#include <std/function.h>

#include <libgfx/render_context.h>
#include <libgfx/framebuffer.h>
#include <libgfx/font.h>

#include <kernel/arch/cpu.h>

struct KeyEvent {
    char ascii;
    u8 scancode;
    u8 modifiers;
};

static int term_cd(int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    int rc = chdir(argv[1]);
    if (rc < 0) {
        return 1;
    }

    return 0;
}

static int term_ls(shell::Terminal& terminal, int argc, char** argv) {
    auto* dir = opendir(argc > 1 ? argv[1] : ".");
    if (!dir) {
        return 1;
    }

    for (;;) {
        struct dirent* entry = readdir(dir);
        if (!entry) {
            break;
        }

        String line;

        line.append(entry->d_name);
        line.append(' ');

        terminal.add_line_without_prompt(move(line));
        terminal.advance_line();
    };

    closedir(dir);
    return 0;
}


int main() {
    int fd = open("/dev/fb0", O_RDONLY);

    FrameBufferResolution resolution;
    ioctl(fd, FB_GET_RESOLUTION, &resolution);

    u32* buffer = reinterpret_cast<u32*>(mmap(nullptr, resolution.width * resolution.pitch, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
    memset(buffer, 0x00, resolution.width * resolution.pitch);

    gfx::FrameBuffer framebuffer(buffer, resolution);
    gfx::RenderContext context(framebuffer);

    OwnPtr<gfx::FontContext> font_context = OwnPtr<gfx::FontContext>::make();
    auto font = gfx::Font::create(*font_context, "/res/fonts/unifont.sfn");

    if (!font) {
        dbgln("Failed to load font");
        return 1;
    }

    int kb = open("/dev/input/keyboard", O_RDONLY);
    KeyEvent event;

    shell::Terminal term(resolution.width, resolution.height, context, font);
    term.on_line_flush = [&term](String text) {
        dbgln("Received line '{}'", text);

        auto cmd = shell::parse_shell_command(text);

        size_t argc = cmd.argc();
        char** argv = cmd.argv();

        if (cmd.name == "cd") {
            term_cd(argc, argv);
        } else if (cmd.name == "ls") {
            term_ls(term, argc, argv);
        } else if (cmd.name == "clear") {
            term.clear();
        }

        for (size_t i = 1; i < argc - 1; i++) {
            delete[] argv[i];
        }
    };

    term.render(16);
    while (true) {
        if (read(kb, &event, sizeof(KeyEvent)) <= 0) {
            continue;
        }

        if (event.scancode & 0x80 || event.ascii == 0) {
            continue;
        }

        term.on_char(event.ascii);
        term.render(16);
    }

    close(fd);
    return 0;
}
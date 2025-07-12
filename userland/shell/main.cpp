#include <shell/terminal.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#include <std/format.h>
#include <std/types.h>
#include <std/vector.h>
#include <std/function.h>

#include <libgfx/render_context.h>
#include <libgfx/framebuffer.h>
#include <libgfx/fonts/psf.h>

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
        terminal.writeln(std::format("ls: {}: {}", argv[1], strerror(errno)));
        return 1;
    }

    for (;;) {
        struct dirent* entry = readdir(dir);
        if (!entry) {
            break;
        }
        
        String line;
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            line.append('/');
        }

        line.append(entry->d_name);
        terminal.writeln(line);
    };

    closedir(dir);
    return 0;
}

static int term_cat(shell::Terminal& terminal, int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        terminal.writeln(std::format("cat: {}: {}", argv[1], strerror(errno)));
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return 1;
    }

    size_t size = st.st_size;
    if (size == 0) {
        close(fd);
        return 0;
    }

    char* buffer = new char[4096];
    while (true) {
        ssize_t n = read(fd, buffer, 4096);
        terminal.write(StringView { buffer, static_cast<size_t>(n) } );

        if (n < 4096) {
            break;
        }
    }

    delete[] buffer;
    close(fd);
    
    return 0;
}

static void spawn(shell::Terminal& terminal, shell::Command const& command, char** argv) {
    int tty = posix_openpt(O_RDWR);

    char* name = ptsname(tty);
    int fd = open(name, O_RDWR);

    pid_t pid = fork();
    if (!pid) {
        close(0);
        close(1);
        close(2);

        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);

        execve(command.pathname(), argv, nullptr);
    }

    char buffer[4096];
    int status = 0;
    
    while (true) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            terminal.write(StringView { buffer, static_cast<size_t>(n) });
        } else if (n < 0) {
            break;
        }
        
        if (waitpid(pid, &status, WNOHANG) > 0) {
            break;
        }
    }

    ssize_t n = read(fd, buffer, sizeof(buffer));
    while (n > 0) {
        terminal.write(StringView { buffer, static_cast<size_t>(n) });
        n = read(fd, buffer, sizeof(buffer));
    }

    close(fd);
}

int main() {
    int gpu = open("/dev/gpu/card0", O_RDONLY);
    if (gpu < 0) {
        dbgln("Failed to open GPU device: {}", strerror(errno));
        return 1;
    }

    struct gpu_connector connector;
    ioctl(gpu, GPU_GET_CONNECTORS, &connector);

    struct gpu_connector_map_fb map_fb;
    map_fb.id = connector.id;

    ioctl(gpu, GPU_CONNECTOR_MAP_FB, &map_fb);
    
    u32* buffer = reinterpret_cast<u32*>(map_fb.framebuffer);
    memset(buffer, 0x00, connector.height * connector.pitch);
    
    gfx::FrameBufferResolution resolution(connector.width, connector.height, connector.pitch);
    
    gfx::FrameBuffer framebuffer(buffer, resolution);
    gfx::RenderContext context(framebuffer);
    
    int kb = open("/dev/input/keyboard0", O_RDONLY);
    KeyEvent event;
    
    auto font = gfx::PSFFont::create("/res/fonts/zap-light16.psf");
    
    shell::Terminal term(connector.width, connector.height, context, font->glyph_data(), font->width(), font->height());
    term.on_line_flush = [&term](String text) {
        if (text.empty()) {
            return;
        }

        dbgln("Received line '{}'", text);
        auto cmd = shell::parse_shell_command(text);

        size_t argc = cmd.argc();
        char** argv = cmd.argv();

        if (cmd.name() == "cd") {
            term_cd(argc, argv);
        } else if (cmd.name() == "ls") {
            term_ls(term, argc, argv);
        } else if (cmd.name() == "cat") {
            term_cat(term, argc, argv);
        } else if (cmd.name() == "clear") {
            term.clear();
        } else {
            struct stat st;
            if (stat(cmd.pathname(), &st) < 0) {
                term.writeln(std::format("{}: command not found", cmd.name()));
                return;
            }

            spawn(term, cmd, argv);
        }

        for (size_t i = 1; i < argc - 1; i++) {
            delete[] argv[i];
        }

        delete[] cmd.pathname();
    };

    while (true) {
        if (read(kb, &event, sizeof(KeyEvent)) <= 0) {
            continue;
        }

        if (event.scancode & 0x80) {
            continue;
        }

        if (!event.ascii) {
            continue;
        }

        term.on_char(event.ascii);
    }

    close(gpu);
    return 0;
}
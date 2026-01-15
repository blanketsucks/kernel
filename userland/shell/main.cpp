#include <shell/terminal.h>
#include <shell/commands.h>

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
#include <std/kmalloc.h>
#include <std/hash_map.h>

#include <libgfx/render_context.h>
#include <libgfx/framebuffer.h>
#include <libgfx/fonts/psf.h>

struct KeyEvent {
    char ascii;
    u8 scancode;
    u8 modifiers;
};

struct BuiltinCommand {
    int(*function)(shell::Terminal&, int, char**);
};

static int term_cd(shell::Terminal& terminal, int argc, char** argv) {
    if (argc < 2) {
        return 1;
    }

    int rc = chdir(argv[1]);
    if (rc < 0) {
        terminal.writeln(std::format("cd: {}: {}", argv[1], strerror(errno)));
        return 1;
    }

    terminal.fetch_cwd();
    return 0;
}

static HashMap<StringView, BuiltinCommand> BUILTIN_COMMANDS = {
    { "cd", { .function = term_cd } }
};

static void spawn(shell::Terminal& terminal, shell::Command const& command, char** argv) {
    int tty = posix_openpt(O_RDWR);

    char* name = ptsname(tty);
    int fd = open(name, O_RDWR);

    if (fd < 0) {
        terminal.writeln(std::format("Failed to open PTY slave {}: {}", name, strerror(errno)));
        close(tty);

        return;
    }

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

static void run_command(shell::Terminal& terminal, String line) {
    String name = shell::parse_command_name(line);
    line = line.substr(name.size() + 1);

    static Vector<String> DEFAULT_BIN_SEARCH_PATH = {
        "/bin", "/usr/bin"
    };

    struct stat st;
    if (name.startswith(".") || name.startswith("..") || name.startswith("/")) {
        int rc = stat_length(name.data(), name.size(), &st);
        if (rc < 0) {
            terminal.writeln(std::format("{}: {}", name, strerror(errno)));
            return;
        }
    } else {
        bool found = false;
        for (auto& path : DEFAULT_BIN_SEARCH_PATH) {
            String fullpath = std::format("{}/{}", path, name);
            int rc = stat_length(fullpath.data(), fullpath.size(), &st);

            if (rc < 0) {
                continue;
            }

            found = true;
            name = fullpath;

            break;
        }

        if (!found) {
            auto iterator = BUILTIN_COMMANDS.find(name);
            if (iterator == BUILTIN_COMMANDS.end()) {
                terminal.writeln(std::format("{}: command not found", name));
                return;
            }

            shell::Command command = shell::parse_command_arguments(name, line);
            iterator->value.function(terminal, command.argc(), command.argv());

            return;
        }
    }

    shell::Command command = shell::parse_command_arguments(name, line);
    spawn(terminal, command, command.argv());
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

    Vector<String> history;
    history.reserve(64);

    auto iterator = history.begin();

    shell::Terminal terminal(context, font);
    terminal.on_line_flush = [&](String text) {
        if (text.empty()) {
            return;
        }

        dbgln("Received line '{}'", text);

        history.append(text);
        if (history.size() > 64) {
            history.remove_first();
        }

        iterator = history.end();
        run_command(terminal, text);
    };

    while (true) {
        if (read(kb, &event, sizeof(KeyEvent)) <= 0) {
            continue;
        }

        if (event.scancode & 0x80) {
            continue;
        }

        if (!event.ascii) {
            if (event.scancode == 0x48) { // Up arrow
                if (iterator != history.begin()) {
                    iterator--;
                    terminal.replace_current_line(*iterator);
                }
            } else if (event.scancode == 0x50) { // Down arrow
                if (iterator != history.end() - 1) {
                    iterator++;
                    terminal.replace_current_line(*iterator);
                }
            }

            continue;
        }

        terminal.on_char(event.ascii);

        struct gpu_connector_flush flush;
        flush.id = connector.id;

        ioctl(gpu, GPU_CONNECTOR_FLUSH, &flush);
    }

    close(gpu);
    return 0;
}
#include "uinput_mouse.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>

UinputMouse::~UinputMouse() { close(); }

bool UinputMouse::open(const char* name)
{
    fd_ = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0) {
        std::fprintf(stderr, "framemouse: cannot open /dev/uinput: %s\n", std::strerror(errno));
        return false;
    }

    bool ok = ioctl(fd_, UI_SET_EVBIT, EV_KEY) == 0
           && ioctl(fd_, UI_SET_KEYBIT, BTN_LEFT) == 0
           && ioctl(fd_, UI_SET_KEYBIT, BTN_RIGHT) == 0
           && ioctl(fd_, UI_SET_KEYBIT, BTN_MIDDLE) == 0
           && ioctl(fd_, UI_SET_EVBIT, EV_REL) == 0
           && ioctl(fd_, UI_SET_RELBIT, REL_X) == 0
           && ioctl(fd_, UI_SET_RELBIT, REL_Y) == 0
           && ioctl(fd_, UI_SET_PROPBIT, INPUT_PROP_POINTER) == 0;

    uinput_setup setup{};
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor = 0x1209;   // pid.codes generic
    setup.id.product = 0xF4A3;
    setup.id.version = 1;
    std::snprintf(setup.name, sizeof(setup.name), "%s", name);

    ok = ok && ioctl(fd_, UI_DEV_SETUP, &setup) == 0 && ioctl(fd_, UI_DEV_CREATE) == 0;
    if (!ok) {
        std::fprintf(stderr, "framemouse: uinput setup failed: %s\n", std::strerror(errno));
        close();
        return false;
    }
    return true;
}

void UinputMouse::close()
{
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        ::close(fd_);
        fd_ = -1;
    }
}

void UinputMouse::emit(int type, int code, int value)
{
    if (fd_ < 0)
        return;
    input_event ev{};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    if (write(fd_, &ev, sizeof(ev)) < 0 && errno != EAGAIN)
        std::fprintf(stderr, "framemouse: uinput write failed: %s\n", std::strerror(errno));
}

void UinputMouse::move(int dx, int dy)
{
    if (dx) emit(EV_REL, REL_X, dx);
    if (dy) emit(EV_REL, REL_Y, dy);
}

void UinputMouse::button(int code, bool pressed) { emit(EV_KEY, code, pressed ? 1 : 0); }

void UinputMouse::sync() { emit(EV_SYN, SYN_REPORT, 0); }
